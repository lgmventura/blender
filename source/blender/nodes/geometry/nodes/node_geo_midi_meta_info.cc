/* SPDX-FileCopyrightText: 2025 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "node_geometry_util.hh"
#include "BKE_attribute.hh"
#include "BKE_pointcloud.hh"

namespace blender::nodes::node_geo_midi_meta_info_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Geometry>("Meta Events"_ustr)
  .description("Point cloud from Read MIDI File (Meta Events output)");

  b.add_output<decl::Float>("Time (s)"_ustr)
      .description("Time of meta event in seconds");
  b.add_output<decl::Float>("Time (qn)"_ustr)
      .description("Time of meta event in quarter notes");
  b.add_output<decl::Float>("BPM"_ustr)
      .description("Tempo in beats per minute (0 if not a tempo event)");
  b.add_output<decl::Int>("Type"_ustr)
      .description("0=Tempo, 1=Time Signature, 2=Key Signature");
  b.add_output<decl::Float>("Time Signature Numerator"_ustr)
      .description("Numerator of time signature (e.g., 4 for 4/4)");
  b.add_output<decl::Float>("Time Signature Denominator"_ustr)
      .description("Denominator of time signature (e.g., 4 for 4/4)");
  b.add_output<decl::Float>("Key Signature"_ustr)
      .description("Key signature: negative=flats, positive=sharps");
  b.add_output<decl::Float>("Key Mode"_ustr)
      .description("0=major, 1=minor");
  b.add_output<decl::Int>("Track"_ustr)
      .description("Track index");
}

static void node_geo_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set = params.extract_input<GeometrySet>("Meta Events"_ustr);
  if (!geometry_set.has_pointcloud()) {
    params.set_default_remaining_outputs();
    return;
  }

  const PointCloud *points = geometry_set.get_pointcloud();
  if (!points) {
    params.set_default_remaining_outputs();
    return;
  }

  params.set_output("Time (s)"_ustr,
                    AttributeFieldInput::from("time_s", CPPType::get<float>()));
  params.set_output("Time (qn)"_ustr,
                    AttributeFieldInput::from("time_qn", CPPType::get<float>()));
  params.set_output("BPM"_ustr,
                    AttributeFieldInput::from("bpm", CPPType::get<float>()));
  params.set_output("Type"_ustr,
                    AttributeFieldInput::from("type", CPPType::get<int>()));
  params.set_output("Time Signature Numerator"_ustr,
                    AttributeFieldInput::from("ts_numerator", CPPType::get<float>()));
  params.set_output("Time Signature Denominator"_ustr,
                    AttributeFieldInput::from("ts_denominator", CPPType::get<float>()));
  params.set_output("Key Signature"_ustr,
                    AttributeFieldInput::from("ks_key", CPPType::get<float>()));
  params.set_output("Key Mode"_ustr,
                    AttributeFieldInput::from("ks_mode", CPPType::get<float>()));
  params.set_output("Track"_ustr,
                    AttributeFieldInput::from("track", CPPType::get<int>()));
}

static void node_register()
{
  static bke::bNodeType ntype;
  bke::node_type_base(ntype, "GeometryNodeMidiMetaInfo"_ustr, NODE_CLASS_GEOMETRY);
  ntype.ui_name = "MIDI Meta Info";
  ntype.ui_description = "Extract MIDI meta event attributes as fields";
  ntype.enum_name_legacy = "MIDI_META_INFO";
  ntype.nclass = NODE_CLASS_GEOMETRY;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_midi_meta_info_cc
