/* SPDX-FileCopyrightText: 2025 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "node_geometry_util.hh"
#include "BKE_attribute.hh"
#include "BKE_pointcloud.hh"

namespace blender::nodes::node_geo_midi_note_info_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Geometry>("Note Events"_ustr)
  .description("Point cloud from Read MIDI File (Note Events output)");

  b.add_output<decl::Float>("Time On (s)"_ustr)
      .description("Start time of each note in seconds");
  b.add_output<decl::Float>("Time On (qn)"_ustr)
      .description("Start time of each note in quarter notes");
  b.add_output<decl::Float>("Duration (s)"_ustr)
      .description("Duration of each note in seconds");
  b.add_output<decl::Float>("Duration (qn)"_ustr)
      .description("Duration of each note in quarter notes");
  b.add_output<decl::Int>("Pitch"_ustr)
      .description("MIDI pitch (0-127)");
  b.add_output<decl::Int>("Velocity"_ustr)
      .description("MIDI velocity (0-127)");
  b.add_output<decl::Int>("Channel"_ustr)
      .description("MIDI channel (0-15)");
  b.add_output<decl::Int>("Track"_ustr)
      .description("Track index");
}

static void node_geo_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set = params.extract_input<GeometrySet>("Note Events"_ustr);
  if (!geometry_set.has_pointcloud()) {
    params.set_default_remaining_outputs();
    return;
  }

  const PointCloud *points = geometry_set.get_pointcloud();
  if (!points) {
    params.set_default_remaining_outputs();
    return;
  }

  /* Use AttributeFieldInput::from and pass GField directly. */
  params.set_output("Time On (s)"_ustr,
                    AttributeFieldInput::from("time_on_s", CPPType::get<float>()));
  params.set_output("Time On (qn)"_ustr,
                    AttributeFieldInput::from("time_on_qn", CPPType::get<float>()));
  params.set_output("Duration (s)"_ustr,
                    AttributeFieldInput::from("duration_s", CPPType::get<float>()));
  params.set_output("Duration (qn)"_ustr,
                    AttributeFieldInput::from("duration_qn", CPPType::get<float>()));
  params.set_output("Pitch"_ustr,
                    AttributeFieldInput::from("pitch", CPPType::get<int>()));
  params.set_output("Velocity"_ustr,
                    AttributeFieldInput::from("velocity", CPPType::get<int>()));
  params.set_output("Channel"_ustr,
                    AttributeFieldInput::from("channel", CPPType::get<int>()));
  params.set_output("Track"_ustr,
                    AttributeFieldInput::from("track", CPPType::get<int>()));
}

static void node_register()
{
  static bke::bNodeType ntype;
  bke::node_type_base(ntype, "GeometryNodeMidiNoteInfo"_ustr, NODE_CLASS_GEOMETRY);
  ntype.ui_name = "MIDI Note Info";
  ntype.ui_description = "Extract MIDI note attributes as fields";
  ntype.enum_name_legacy = "MIDI_NOTE_INFO";
  ntype.nclass = NODE_CLASS_GEOMETRY;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_midi_note_info_cc
