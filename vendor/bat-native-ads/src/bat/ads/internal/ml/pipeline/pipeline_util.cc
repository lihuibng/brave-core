/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/ml_aliases.h"
#include "bat/ads/internal/ml/ml_transformation_util.h"
#include "bat/ads/internal/ml/pipeline/pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/pipeline_util.h"

namespace ads {
namespace ml {
namespace pipeline {

base::Optional<TransformationVector> ParsePipelineTransformations(
    base::Value* transformations_value) {
  if (!transformations_value || !transformations_value->is_list()) {
    return base::nullopt;
  }

  base::Optional<TransformationVector> transformations = TransformationVector();
  for (const base::Value& transformation : transformations_value->GetList()) {
    const std::string* transformation_type =
        transformation.FindStringKey("transformation_type");

    if (!transformation_type) {
      return base::nullopt;
    }

    std::string parsed_transformation_type = *transformation_type;

    if (parsed_transformation_type.compare("TO_LOWER") == 0) {
      transformations.value().push_back(
          std::make_unique<LowercaseTransformation>(LowercaseTransformation()));
    }

    if (parsed_transformation_type.compare("NORMALIZE") == 0) {
      transformations.value().push_back(
          std::make_unique<NormalizationTransformation>(
              NormalizationTransformation()));
    }

    if (parsed_transformation_type.compare("HASHED_NGRAMS") == 0) {
      const base::Value* transformation_params =
          transformation.FindKey("params");

      if (!transformation_params) {
        return base::nullopt;
      }

      const base::Optional<int> nb =
          transformation_params->FindIntKey("num_buckets");
      if (!nb.has_value()) {
        return base::nullopt;
      }

      int num_buckets = nb.value();

      const base::Value* ngram_sizes =
          transformation_params->FindListKey("ngrams_range");

      if (!ngram_sizes) {
        return base::nullopt;
      }

      std::vector<int> ngram_range;
      for (const base::Value& n : ngram_sizes->GetList()) {
        if (n.is_int()) {
          ngram_range.push_back(n.GetInt());
        } else {
          return base::nullopt;
        }
      }
      HashedNGramsTransformation hashed_ngrams(num_buckets, ngram_range);
      transformations.value().push_back(
          std::make_unique<HashedNGramsTransformation>(hashed_ngrams));
    }
  }

  return transformations;
}

base::Optional<model::Linear> ParsePipelineClassifier(
    base::Value* classifier_value) {
  if (!classifier_value) {
    return base::nullopt;
  }

  std::string* classifier_type =
      classifier_value->FindStringKey("classifier_type");

  if (!classifier_type) {
    return base::nullopt;
  }

  std::string parsed_classifier_type = *classifier_type;

  if (parsed_classifier_type.compare("LINEAR")) {
    return base::nullopt;
  }

  base::Value* specified_classes = classifier_value->FindListKey("classes");
  if (!specified_classes) {
    return base::nullopt;
  }

  std::vector<std::string> classes;
  for (const base::Value& class_name : specified_classes->GetList()) {
    if (class_name.is_string()) {
      classes.push_back(class_name.GetString());
    } else {
      return base::nullopt;
    }
  }

  base::Value* class_weights = classifier_value->FindDictKey("class_weights");
  if (!class_weights) {
    return base::nullopt;
  }

  std::map<std::string, VectorData> weights;
  for (const std::string& class_string : classes) {
    base::Value* this_class = class_weights->FindListKey(class_string);
    if (!this_class) {
      return base::nullopt;
    }
    std::vector<double> class_coef_weights;
    for (const base::Value& weight : this_class->GetList()) {
      if (weight.is_double() || weight.is_int()) {
        class_coef_weights.push_back(weight.GetDouble());
      } else {
        return base::nullopt;
      }
    }
    weights[class_string] = VectorData(class_coef_weights);
  }

  std::map<std::string, double> specified_biases;
  base::Value* biases = classifier_value->FindListKey("biases");
  if (!biases) {
    return base::nullopt;
  }

  auto biases_list = biases->GetList();
  if (biases_list.size() != classes.size()) {
    return base::nullopt;
  }

  for (size_t i = 0; i < biases_list.size(); i++) {
    const base::Value& this_bias = biases_list[i];
    if (this_bias.is_double() || this_bias.is_int()) {
      specified_biases[classes[i]] = this_bias.GetDouble();
    } else {
      return base::nullopt;
    }
  }

  base::Optional<model::Linear> linear_model =
      model::Linear(weights, specified_biases);
  return linear_model;
}

base::Optional<PipelineInfo> ParsePipelineJSON(const std::string& json) {
  base::Optional<base::Value> root = base::JSONReader::Read(json);

  if (!root) {
    return base::nullopt;
  }

  base::Optional<int> version_value = root->FindIntKey("version");
  if (!version_value.has_value()) {
    return base::nullopt;
  }
  int version = version_value.value();

  std::string* timestamp_value = root->FindStringKey("timestamp");
  if (!timestamp_value) {
    return base::nullopt;
  }
  std::string timestamp = *timestamp_value;

  std::string* locale_value = root->FindStringKey("locale");
  if (!locale_value) {
    return base::nullopt;
  }
  std::string locale = *locale_value;

  base::Optional<TransformationVector> transformations_optional =
      ParsePipelineTransformations(root->FindListKey("transformations"));
  if (!transformations_optional.has_value()) {
    return base::nullopt;
  }

  const base::Optional<model::Linear> linear_model_optional =
      ParsePipelineClassifier(root->FindKey("classifier"));
  if (!linear_model_optional.has_value()) {
    return base::nullopt;
  }

  TransformationVector transformations =
      GetTransformationVectorDeepCopy(transformations_optional.value());

  const model::Linear linear_model = linear_model_optional.value();

  base::Optional<PipelineInfo> pipeline_info =
      PipelineInfo(version, timestamp, locale, transformations, linear_model);

  return pipeline_info;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
