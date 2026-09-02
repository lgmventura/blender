/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_pointcloud.hh"
#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_midi_filter_track_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Geometry>("Events"_ustr)
  .description("Point cloud from Read MIDI File node");
  b.add_input<decl::Int>("Track"_ustr)
      .default_value(0)
      .min(0)
      .description("Track index to keep (0-based)");
  b.add_output<decl::Geometry>("Events"_ustr)
      .description("Point cloud containing only events from the selected track");
}

static void node_geo_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set = params.extract_input<GeometrySet>("Events"_ustr);
  const int track = params.extract_input<int>("Track"_ustr);

  if (!geometry_set.has_pointcloud()) {
    params.set_default_remaining_outputs();
    return;
  }

  const PointCloud *src_points = geometry_set.get_pointcloud();
  if (!src_points) {
    params.set_default_remaining_outputs();
    return;
  }

  const bke::AttributeAccessor attributes = src_points->attributes();
  const VArray<int> track_attr = *attributes.lookup_or_default<int>("track", AttrDomain::Point, -1);

  /* Collect indices of points that match the track. */
  Vector<int> indices;
  for (const int i : IndexRange(src_points->totpoint)) {
    if (track_attr[i] == track) {
      indices.append(i);
    }
  }

  if (indices.is_empty()) {
    /* Output empty geometry. */
    GeometrySet result;
    result.replace_pointcloud(BKE_pointcloud_new_nomain(0));
    params.set_output("Events"_ustr, std::move(result));
    return;
  }

  /* Create a new point cloud. */
  PointCloud *filtered_points = BKE_pointcloud_new_nomain(indices.size());
  MutableSpan<float3> dst_positions = filtered_points->positions_for_write();
  const Span<float3> src_positions = src_points->positions();

  /* Copy positions. */
  for (const int i : indices.index_range()) {
    dst_positions[i] = src_positions[indices[i]];
  }

  /* Copy known MIDI attributes (only those we care about). */
  bke::MutableAttributeAccessor dst_attributes = filtered_points->attributes_for_write();

  /* Helper lambda to copy an attribute. */
  auto copy_attribute = [&](const char *name) {
    const VArray<int> src_attr = *attributes.lookup_or_default<int>(name, AttrDomain::Point, 0);
    bke::SpanAttributeWriter<int> dst_attr =
        dst_attributes.lookup_or_add_for_write_only_span<int>(name, AttrDomain::Point);
    if (dst_attr) {
      for (const int i : indices.index_range()) {
        dst_attr.span[i] = src_attr[indices[i]];
      }
      dst_attr.finish();
    }
  };

  /* Copy integer attributes. */
  copy_attribute("track");
  copy_attribute("channel");
  copy_attribute("pitch");
  copy_attribute("velocity");

  /* Copy float attributes. */
  auto copy_float_attribute = [&](const char *name) {
    const VArray<float> src_attr = *attributes.lookup_or_default<float>(name, AttrDomain::Point, 0.0f);
    bke::SpanAttributeWriter<float> dst_attr =
        dst_attributes.lookup_or_add_for_write_only_span<float>(name, AttrDomain::Point);
    if (dst_attr) {
      for (const int i : indices.index_range()) {
        dst_attr.span[i] = src_attr[indices[i]];
      }
      dst_attr.finish();
    }
  };

  copy_float_attribute("time_on_s");
  copy_float_attribute("time_on_qn");
  copy_float_attribute("duration_s");
  copy_float_attribute("duration_qn");

  copy_float_attribute("bpm");
  copy_float_attribute("ts_numerator");
  copy_float_attribute("ts_denominator");
  copy_float_attribute("ks_key");
  copy_float_attribute("ks_mode");



  GeometrySet result;
  result.replace_pointcloud(filtered_points);
  params.set_output("Events"_ustr, std::move(result));
}

// if ones drags the Note Events output from Read MIDI file and drops in
// a blank area from Geometry Nodes, a dialogue appears to search for the
// next node. This register is needed so this node is found there.
static void node_gather_link_search_ops(GatherLinkSearchOpParams &params)
{
  const NodeDeclaration &declaration = *params.node_type().static_declaration;
  search_link_ops_for_declarations(params, declaration.inputs);
}

static void node_register()
{
  static bke::bNodeType ntype;
  bke::node_type_base(ntype, "GeometryNodeMidiFilterTrack"_ustr, NODE_CLASS_GEOMETRY);
  ntype.ui_name = "Filter MIDI Track";
  ntype.ui_description = "Filter MIDI events by track index";
  ntype.enum_name_legacy = "MIDI_FILTER_TRACK";
  ntype.nclass = NODE_CLASS_GEOMETRY;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  ntype.gather_link_search_ops = node_gather_link_search_ops;
  bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_midi_filter_track_cc
