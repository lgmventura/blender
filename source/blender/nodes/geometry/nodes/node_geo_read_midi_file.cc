/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_mesh.hh"
#include "BLI_math_vector_types.hh"
#include "node_geometry_util.hh"
#include "BKE_pointcloud.hh"

#include "MidiFile.h"


namespace blender::nodes::node_geo_read_midi_file_cc {

static void node_declare(NodeDeclarationBuilder &b) {
  b.add_input<decl::String>("File Path"_ustr)
      .subtype(PROP_FILEPATH)
      .default_value("")
      .description("Path to MIDI file");
  b.add_output<decl::Geometry>("Events"_ustr)
      .description("Point cloud of raw MIDI events");
}

static void node_geo_exec(GeoNodeExecParams params) {
  const std::string file_path = params.extract_input<std::string>("File Path"_ustr);
  if (file_path.empty()) { params.set_default_remaining_outputs(); return; }

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

  int tpq = midifile.getTicksPerQuarterNote();
  int total_events = 0;
  if (total_events == 0) {
    params.error_message_add(NodeWarningType::Warning, "MIDI file contains no events");
    params.set_default_remaining_outputs();
    return;
  }

  for (int t=0; t<midifile.getTrackCount(); t++) total_events += midifile[t].size();

  blender::PointCloud *points = BKE_pointcloud_new_nomain(total_events);
  MutableSpan<float3> positions = points->positions_for_write();
  // Atributos: track, type, time_ticks, pitch, velocity, channel, value (float), data (int), tpq (int)

  int idx = 0;
  for (int t=0; t<midifile.getTrackCount(); t++) {
    for (int e=0; e<midifile[t].size(); e++) {
      smf::MidiEvent &ev = midifile[t][e];
      // Preencher posições (ex: idx,0,0) e atributos
      // ...
      idx++;
    }
  }

  GeometrySet geometry_set;
  geometry_set.replace_pointcloud(points);
  params.set_output("Events"_ustr, std::move(geometry_set));
}

static void node_register() {
  static bke::bNodeType ntype;
  bke::node_type_base(ntype, "GeometryNodeReadMidiFile"_ustr, NODE_CLASS_GEOMETRY);
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
