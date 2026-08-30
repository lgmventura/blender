/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_mesh.hh"
#include "BLI_math_vector_types.hh"
#include "node_geometry_util.hh"
#include "BKE_pointcloud.hh"

#include "MidiFile.h"


namespace blender::nodes::node_geo_midi_read_file_cc {

enum class MidiEventType {
  NoteOff = 0,
  NoteOn = 1,
  Aftertouch = 2,
  Controller = 3,
  Timbre = 4,
  Pressure = 5,
  Pitchbend = 6,
  Tempo = 7,
  TimeSignature = 8,
  KeySignature = 9,
};


static void node_declare(NodeDeclarationBuilder &b) {
  b.add_input<decl::String>("File Path"_ustr)
      .subtype(PROP_FILEPATH)
      .default_value("")
      .description("Path to MIDI file");
  b.add_output<decl::Geometry>("Note Events"_ustr)
      .description("Point cloud of paired MIDI note events");
  b.add_output<decl::Geometry>("Meta Events"_ustr)
      .description("Point cloud of MIDI meta events, such as tempo and time signatures");
}
static void node_geo_exec(GeoNodeExecParams params) {
  const std::string file_path = params.extract_input<std::string>("File Path"_ustr);
  if (file_path.empty()) {
    params.set_default_remaining_outputs();
    return;
  }

  smf::MidiFile midifile;
  try {
    if (!midifile.read(file_path)) {
      params.error_message_add(NodeWarningType::Error, "Failed to read MIDI file (invalid format): " + file_path);
      params.set_default_remaining_outputs();
      return;
    }
  }
  catch (const std::exception &e) {
    params.error_message_add(NodeWarningType::Error, "MIDI read exception: " + std::string(e.what()));
    params.set_default_remaining_outputs();
    return;
  }

  midifile.doTimeAnalysis();
  midifile.linkNotePairs();

  const int tpq = midifile.getTicksPerQuarterNote();

          // Contar notas (note_on) e meta eventos (tempo, time signature, etc.)
  int note_count = 0;
  int meta_count = 0;
  for (int t = 0; t < midifile.getTrackCount(); t++) {
    for (int e = 0; e < midifile[t].size(); e++) {
      smf::MidiEvent &ev = midifile[t][e];
      if (ev.isNoteOn()) {
        note_count++;
      }
      else if (ev.isMeta()) {
        if (ev.isTempo() || ev[1] == 0x58 || ev[1] == 0x59) { // tempo, time signature, key signature,
          meta_count++;
        }
      }
    }
  }

          // Se não houver dados, retorna vazio
  if (note_count == 0 && meta_count == 0) {
    params.error_message_add(NodeWarningType::Warning, "MIDI file contains no notes or meta events");
    params.set_default_remaining_outputs();
    return;
  }

  /* ---------- POINT CLOUD OF NOTES ---------- */
  PointCloud *notes_points = nullptr;
  if (note_count > 0) {
    notes_points = BKE_pointcloud_new_nomain(note_count);
    MutableSpan<float3> positions = notes_points->positions_for_write();
    bke::MutableAttributeAccessor attributes = notes_points->attributes_for_write();

    bke::SpanAttributeWriter<int> pitch_attr =
        attributes.lookup_or_add_for_write_only_span<int>("pitch", AttrDomain::Point);
    bke::SpanAttributeWriter<int> velocity_attr =
        attributes.lookup_or_add_for_write_only_span<int>("velocity", AttrDomain::Point);
    bke::SpanAttributeWriter<float> time_on_s_attr =
        attributes.lookup_or_add_for_write_only_span<float>("time_on_s", AttrDomain::Point);
    bke::SpanAttributeWriter<float> time_on_qn_attr =
        attributes.lookup_or_add_for_write_only_span<float>("time_on_qn", AttrDomain::Point);
    bke::SpanAttributeWriter<float> duration_s_attr =
        attributes.lookup_or_add_for_write_only_span<float>("duration_s", AttrDomain::Point);
    bke::SpanAttributeWriter<float> duration_qn_attr =
        attributes.lookup_or_add_for_write_only_span<float>("duration_qn", AttrDomain::Point);
    bke::SpanAttributeWriter<int> channel_attr =
        attributes.lookup_or_add_for_write_only_span<int>("channel", AttrDomain::Point);
    bke::SpanAttributeWriter<int> track_attr =
        attributes.lookup_or_add_for_write_only_span<int>("track", AttrDomain::Point);

    int idx = 0;
    for (int t = 0; t < midifile.getTrackCount(); t++) {
      for (int e = 0; e < midifile[t].size(); e++) {
        smf::MidiEvent &ev = midifile[t][e];
        if (ev.isNoteOn()) {
          positions[idx] = float3(float(ev.seconds), float(ev.getP1()), float(ev.getChannel()));
          pitch_attr.span[idx] = ev.getP1();
          velocity_attr.span[idx] = ev.getP2();
          time_on_s_attr.span[idx] = float(ev.seconds);
          time_on_qn_attr.span[idx] = float(ev.tick) / float(tpq);
          duration_s_attr.span[idx] = ev.getDurationInSeconds();
          duration_qn_attr.span[idx] = float(ev.getTickDuration()) / float(tpq);
          channel_attr.span[idx] = ev.getChannel();
          track_attr.span[idx] = t;
          idx++;
        }
      }
    }

    pitch_attr.finish();
    velocity_attr.finish();
    time_on_s_attr.finish();
    time_on_qn_attr.finish();
    duration_s_attr.finish();
    duration_qn_attr.finish();
    channel_attr.finish();
    track_attr.finish();
  }

  /* ---------- POINT CLOUD OF META EVENTS ---------- */
  PointCloud *meta_points = nullptr;
  if (meta_count > 0) {
    meta_points = BKE_pointcloud_new_nomain(meta_count);
    MutableSpan<float3> positions_meta = meta_points->positions_for_write();
    bke::MutableAttributeAccessor attributes_meta = meta_points->attributes_for_write();

    // Attributes for meta events
    bke::SpanAttributeWriter<int> meta_type_attr =
        attributes_meta.lookup_or_add_for_write_only_span<int>("type", AttrDomain::Point);
    bke::SpanAttributeWriter<float> time_s_attr =
        attributes_meta.lookup_or_add_for_write_only_span<float>("time_s", AttrDomain::Point);
    bke::SpanAttributeWriter<float> time_qn_attr =
        attributes_meta.lookup_or_add_for_write_only_span<float>("time_qn", AttrDomain::Point);
    bke::SpanAttributeWriter<float> bpm_attr =
        attributes_meta.lookup_or_add_for_write_only_span<float>("bpm", AttrDomain::Point);
    bke::SpanAttributeWriter<float> ts_numerator =
        attributes_meta.lookup_or_add_for_write_only_span<float>("ts_numerator", AttrDomain::Point);
    bke::SpanAttributeWriter<float> ts_denominator =
        attributes_meta.lookup_or_add_for_write_only_span<float>("ts_denominator", AttrDomain::Point);
    bke::SpanAttributeWriter<float> ks_key =
        attributes_meta.lookup_or_add_for_write_only_span<float>("ks_key", AttrDomain::Point);
    bke::SpanAttributeWriter<float> ks_mode =
        attributes_meta.lookup_or_add_for_write_only_span<float>("ks_mode", AttrDomain::Point);
    bke::SpanAttributeWriter<int> channel_attr =
        attributes_meta.lookup_or_add_for_write_only_span<int>("channel", AttrDomain::Point);
    bke::SpanAttributeWriter<int> track_attr =
        attributes_meta.lookup_or_add_for_write_only_span<int>("track", AttrDomain::Point);
    // more...

    int idx_meta = 0;
    for (int t = 0; t < midifile.getTrackCount(); t++) {
      for (int e = 0; e < midifile[t].size(); e++) {
        smf::MidiEvent &ev = midifile[t][e];
        if (ev.isMeta()) {
          if (ev.isTempo()) {
            positions_meta[idx_meta] = float3(float(ev.seconds), 0.0f, 0.0f);
            meta_type_attr.span[idx_meta] = int(MidiEventType::Tempo); // 0 = tempo
            time_s_attr.span[idx_meta] = float(ev.seconds);
            time_qn_attr.span[idx_meta] = float(ev.tick) / float(tpq);
            bpm_attr.span[idx_meta] = ev.getTempoBPM();
            channel_attr.span[idx_meta] = ev.getChannel();
            track_attr.span[idx_meta] = t;
            idx_meta++;
          }
          else if (ev[1] == 0x58) { // Time signature
            int numerator = ev[2];
            int denominator_power = ev[3]; // 2^denominator_power = denominador
            positions_meta[idx_meta] = float3(float(ev.seconds), 1.0f, 0.0f);
            meta_type_attr.span[idx_meta] = int(MidiEventType::TimeSignature); // time signature
            time_s_attr.span[idx_meta] = float(ev.seconds);
            time_qn_attr.span[idx_meta] = float(ev.tick) / float(tpq);
            ts_numerator.span[idx_meta] = numerator;
            ts_denominator.span[idx_meta] = 1 << denominator_power; // std::pow(2.0f, denominator_power);
            channel_attr.span[idx_meta] = ev.getChannel();
            track_attr.span[idx_meta] = t;
            idx_meta++;
          }
          else if (ev[1] == 0x59) { // key signature
            int key = ev[2]; // -7 a +7 (negative = flats, positive = sharps)
            int mode = ev[3]; // 0 = major, 1 = minor
            positions_meta[idx_meta] = float3(float(ev.seconds), 2.0f, 0.0f);
            meta_type_attr.span[idx_meta] = int(MidiEventType::KeySignature); // key signature
            time_s_attr.span[idx_meta] = float(ev.seconds);
            time_qn_attr.span[idx_meta] = float(ev.tick) / float(tpq);
            ks_key.span[idx_meta] = key;
            ks_mode.span[idx_meta] = mode;
            channel_attr.span[idx_meta] = ev.getChannel();
            track_attr.span[idx_meta] = t;
            idx_meta++;
          }
        }
      }
    }

    meta_type_attr.finish();
    time_s_attr.finish();
    time_qn_attr.finish();
    bpm_attr.finish();
    ts_numerator.finish();
    ts_denominator.finish();
    ks_key.finish();
    ks_mode.finish();
  }

  /* ---------- Outputs ---------- */
  GeometrySet notes_geometry;
  if (notes_points) {
    notes_geometry.replace_pointcloud(notes_points);
  }
  params.set_output("Note Events"_ustr, std::move(notes_geometry));

  GeometrySet meta_geometry;
  if (meta_points) {
    meta_geometry.replace_pointcloud(meta_points);
  }
  params.set_output("Meta Events"_ustr, std::move(meta_geometry));
}

static void node_register() {
  static bke::bNodeType ntype;
  bke::node_type_base(ntype, "GeometryNodeMidiReadFile"_ustr, NODE_CLASS_GEOMETRY);
  ntype.ui_name = "Read MIDI File";
  ntype.ui_description = "Extract raw MIDI events as a point cloud";
  ntype.enum_name_legacy = "READ_MIDI_FILE";
  ntype.nclass = NODE_CLASS_GEOMETRY;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}
