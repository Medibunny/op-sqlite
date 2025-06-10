#pragma once

#include "DumbHostObject.h"
#include "SmartHostObject.h"
#include "types.h"
#include <jsi/jsi.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace opsqlite {

namespace jsi = facebook::jsi;

jsi::Value to_jsi(jsi::Runtime &rt, const JSVariant &value);

JSVariant to_variant(jsi::Runtime &rt, jsi::Value const &value);

std::vector<std::string> to_string_vec(jsi::Runtime &rt, jsi::Value const &xs);

std::vector<JSVariant> to_variant_vec(jsi::Runtime &rt, jsi::Value const &xs);

std::vector<int> to_int_vec(jsi::Runtime &rt, jsi::Value const &xs);

jsi::Value
create_result(jsi::Runtime &rt, const BridgeResult &status,
              std::vector<DumbHostObject> *results,
              std::shared_ptr<std::vector<SmartHostObject>> metadata);

jsi::Value create_js_rows(jsi::Runtime &rt, const BridgeResult &status);

jsi::Value
create_raw_result(jsi::Runtime &rt, const BridgeResult &status,
                  const std::vector<std::vector<JSVariant>> *results);

void to_batch_arguments(jsi::Runtime &rt, jsi::Array const &batch_params,
                        std::vector<BatchArguments> *commands);

bool folder_exists(const std::string &name);

bool file_exists(const std::string &path);

void log_to_console(jsi::Runtime &rt, const std::string &message);

std::vector<BatchArguments> to_batch_arguments(jsi::Runtime &rt,
                                               const jsi::Array &batchParams);

#ifndef OP_SQLITE_USE_LIBSQL
ImportResult import_sql_file(sqlite3 *db, const std::string &fileName);
#endif

std::string zstd_compress_file(const std::string &fileName);

} // namespace opsqlite
