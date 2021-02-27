/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/sql_store.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"

namespace ledger {

namespace {

void NormalizeResponse(mojom::DBCommandResponse* response) {
  DCHECK(response);
  if (!response->result) {
    auto result = mojom::DBCommandResult::New();
    result->set_records({});
    response->result = std::move(result);
  } else if (response->result->is_value()) {
    auto record = mojom::DBRecord::New();
    record->fields.push_back(response->result->get_value()->Clone());

    std::vector<mojom::DBRecordPtr> records;
    records.push_back(std::move(record));

    auto result = mojom::DBCommandResult::New();
    result->set_records(std::move(records));
    response->result = std::move(result);
  }
}

}  // namespace

SQLReader::SQLReader(const mojom::DBCommandResponse* response)
    : response_(response) {
  DCHECK(response_);
}

SQLReader::SQLReader(const mojom::DBCommandResponsePtr& response)
    : SQLReader(response.get()) {}

SQLReader::~SQLReader() = default;

bool SQLReader::Step() {
  if (!response_->result || !response_->result->is_records())
    return false;

  int record_count = static_cast<int>(response_->result->get_records().size());
  if (row_ >= record_count)
    return false;

  return ++row_ < record_count;
}

bool SQLReader::Succeeded() const {
  return response_->status == mojom::DBCommandResponse::Status::RESPONSE_OK;
}

bool SQLReader::ColumnBool(int col) const {
  return static_cast<bool>(ColumnInt64(col));
}

int64_t SQLReader::ColumnInt64(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value)
    return 0;

  switch (db_value->which()) {
    case mojom::DBValue::Tag::NULL_VALUE:
      return 0;
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::INT_VALUE:
      return static_cast<int64_t>(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return db_value->get_int64_value();
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return static_cast<int64_t>(db_value->get_double_value());
    case mojom::DBValue::Tag::STRING_VALUE: {
      int64_t value = 0;
      base::StringToInt64(db_value->get_string_value(), &value);
      return value;
    }
  }
}

double SQLReader::ColumnDouble(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value)
    return 0;

  switch (db_value->which()) {
    case mojom::DBValue::Tag::NULL_VALUE:
      return 0;
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? 1 : 0;
    case mojom::DBValue::Tag::INT_VALUE:
      return static_cast<double>(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return static_cast<double>(db_value->get_int64_value());
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return db_value->get_double_value();
    case mojom::DBValue::Tag::STRING_VALUE: {
      double value = 0;
      base::StringToDouble(db_value->get_string_value(), &value);
      return value;
    }
  }
}

std::string SQLReader::ColumnString(int col) const {
  auto* db_value = GetDBValue(col);
  if (!db_value)
    return "";

  switch (db_value->which()) {
    case mojom::DBValue::Tag::BOOL_VALUE:
      return db_value->get_bool_value() ? "1" : "0";
    case mojom::DBValue::Tag::INT_VALUE:
      return base::NumberToString(db_value->get_int_value());
    case mojom::DBValue::Tag::INT64_VALUE:
      return base::NumberToString(db_value->get_int64_value());
    case mojom::DBValue::Tag::DOUBLE_VALUE:
      return base::NumberToString(db_value->get_double_value());
    case mojom::DBValue::Tag::STRING_VALUE:
      return db_value->get_string_value();
    case mojom::DBValue::Tag::NULL_VALUE:
      return "";
  }
}

mojom::DBValue* SQLReader::GetDBValue(int col) const {
  if (!response_->result || !response_->result->is_records())
    return nullptr;

  auto& records = response_->result->get_records();
  if (row_ >= static_cast<int>(records.size()))
    return nullptr;

  auto& record = records[row_];
  int column_count = static_cast<int>(record->fields.size());
  if (col < 0 || col >= column_count)
    return nullptr;

  return record->fields[col].get();
}

const BATLedgerContext::ComponentKey SQLStore::kComponentKey;

SQLStore::SQLStore(BATLedgerContext* context) : Component(context) {}

void SQLStore::SetDBValue(mojom::DBValue* db_value, double value) {
  db_value->set_double_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, int32_t value) {
  db_value->set_int64_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, int64_t value) {
  db_value->set_int64_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, bool value) {
  db_value->set_int64_value(value ? 1 : 0);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, const char* value) {
  db_value->set_string_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, const std::string& value) {
  db_value->set_string_value(value);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, std::nullptr_t) {
  db_value->set_null_value(0);
}

void SQLStore::SetDBValue(mojom::DBValue* db_value, mojom::DBValuePtr value) {
  *db_value = *value;
}

SQLStore::CommandResult SQLStore::RunTransaction(
    mojom::DBTransactionPtr transaction) {
  return RunTransactionImpl(std::move(transaction));
}

SQLStore::CommandResult SQLStore::RunTransaction(mojom::DBCommandPtr command) {
  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));
  return RunTransactionImpl(std::move(transaction));
}

SQLStore::CommandResult SQLStore::RunTransactionImpl(
    mojom::DBTransactionPtr transaction) {
  DCHECK(transaction);

  CommandResult::Resolver resolver;
  context()->GetLedgerClient()->RunDBTransaction(
      std::move(transaction),
      [resolver](mojom::DBCommandResponsePtr response) mutable {
        NormalizeResponse(response.get());
        resolver.Complete(std::move(response));
      });

  return resolver.result();
}

}  // namespace ledger
