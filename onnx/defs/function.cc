// Copyright (c) Facebook Inc. and Microsoft Corporation.
// Licensed under the MIT license.

#include "onnx/defs/function.h"
#include "onnx/string_utils.h"

namespace ONNX_NAMESPACE {
std::string InteralTensorNameGenerator(
    const std::string& node_name,
    const std::string& internal_name) {
  std::string new_name = "Func_" + node_name + internal_name;
  return new_name;
}

void FunctionExpandHelper(
    const NodeProto& node,
    const FunctionProto& func,
    GraphProto& g,
    const std::string& node_prefix) {
  // Create a temporary unique node prefix for tensor names
  std::string uniq_prefix = node_prefix;
  if (uniq_prefix.empty()) {
    const void* address = static_cast<const void*>(&node);
    std::stringstream ss;
    ss << address;
    uniq_prefix = ss.str();
  }
  std::string node_name =
      node.has_name() ? node.name() : func.name() + uniq_prefix;
  std::unordered_map<std::string, std::string> input_names_map;
  std::unordered_map<std::string, std::string> output_names_map;
  std::unordered_map<std::string, AttributeProto> attr_map;

  for (int idx = 0; idx < node.input_size(); ++idx) {
    if (idx >= func.input_size()) {
      throw std::runtime_error(
          "Input for function node " + node_name + " is out of bounds");
    }
    input_names_map[func.input().Get(idx)] = node.input().Get(idx);
  }
  for (int idx = 0; idx < node.output_size(); ++idx) {
    if (idx >= func.output_size()) {
      throw std::runtime_error(
          "Output for function node " + node_name + " is out of bounds");
    }
    output_names_map[func.output().Get(idx)] = node.output().Get(idx);
  }

  for (auto& attr : node.attribute()) {
    attr_map[attr.name()] = attr;
  }

  for (auto& function_node : func.node()) {
    NodeProto* new_node = g.add_node();
    new_node->CopyFrom(function_node);
    new_node->clear_input();
    new_node->clear_output();
    new_node->clear_attribute();
    for (auto& input : function_node.input()) {
      if (input_names_map.count(input)) {
        new_node->add_input(input_names_map[input]);
      } else {
        new_node->add_input(InteralTensorNameGenerator(node_name, input));
      }
    }
    for (auto& output : function_node.output()) {
      if (output_names_map.count(output)) {
        new_node->add_output(output_names_map[output]);
      } else {
        new_node->add_output(InteralTensorNameGenerator(node_name, output));
      }
    }
    for (auto& attr : function_node.attribute()) {
      if (attr.has_ref_attr_name()) {
        if (attr_map.count(attr.ref_attr_name())) {
          AttributeProto* new_attr = new_node->add_attribute();
          new_attr->CopyFrom(attr_map[attr.ref_attr_name()]);
        }
      } else {
        AttributeProto* new_attr = new_node->add_attribute();
        new_attr->CopyFrom(attr);
      }
    }
  }
}

FunctionProto FunctionProtoHelper::Define(
    const std::string& name,
    int since_version,
    std::vector<std::string> inputs,
    std::vector<std::string> outputs,
    std::vector<std::string> attributes,
    std::vector<NodeDef> node_defs) {
  FunctionProto func;

  func.set_name(name);
  func.set_since_version(since_version);

  for (const auto& i : inputs) {
    func.add_input(i);
  }
  for (const auto& o : outputs) {
    func.add_output(o);
  }
  for (const auto& a : attributes) {
    func.add_attribute(a);
  }

  for (const auto& node : node_defs) {
    NodeProto* n = func.add_node();

    n->set_op_type(node.op_type);

    for (const auto& i : node.inputs) {
      n->add_input(i);
    }

    for (const auto& o : node.outputs) {
      n->add_output(o);
    }

    for (const auto& attr_pair : node.attributes) {
      AttributeProto* attr = n->add_attribute();
      *attr = attr_pair.second.proto;
      attr->set_name(attr_pair.first);
    }
  }

  return func;
}

std::unordered_map<std::string, AttributeProto_AttributeType>
    FunctionProtoHelper ::AttributeProtoWrapper::attr_name_map = {
        {"float", AttributeProto_AttributeType_FLOAT},
        {"int", AttributeProto_AttributeType_INT},
        {"string", AttributeProto_AttributeType_STRING},
        {"tensor", AttributeProto_AttributeType_TENSOR},
        {"graph", AttributeProto_AttributeType_GRAPH},
        {"floats", AttributeProto_AttributeType_FLOATS},
        {"ints", AttributeProto_AttributeType_INTS},
        {"strings", AttributeProto_AttributeType_STRINGS},
        {"tensors", AttributeProto_AttributeType_TENSORS},
        {"graphs", AttributeProto_AttributeType_GRAPHS}};

void FunctionProtoHelper::AttributeProtoWrapper::InitFromString(
    const std::string& val) {
  if (val.size() >= 2 && val[0] == '$') {
    std::size_t found = val.find(':');

    proto.set_ref_attr_name(val.substr(1, found - 1));

    std::string type = val.substr(found + 1, val.size() - found - 1);

    auto it = attr_name_map.find(type);
    if (it != attr_name_map.end()) {
      proto.set_type(it->second);
    } else {
      throw std::exception();
    }

  } else {
    // set as string
    SetAttrValue(val, &proto);
  }
}

} // namespace ONNX_NAMESPACE
