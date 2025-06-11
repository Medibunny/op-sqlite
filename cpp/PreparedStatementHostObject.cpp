#include "PreparedStatementHostObject.h"
#if OP_SQLITE_USE_LIBSQL
#include "libsql/bridge.h"
#else
#include "bridge.h"
#endif
#include "macros.h"
#include "utils.h"

namespace opsqlite {

namespace jsi = facebook::jsi;

std::vector<jsi::PropNameID>
PreparedStatementHostObject::getPropertyNames(jsi::Runtime &rt) {
    std::vector<jsi::PropNameID> keys;

    return keys;
}

jsi::Value PreparedStatementHostObject::get(jsi::Runtime &rt,
                                            const jsi::PropNameID &propNameID) {
    auto name = propNameID.utf8(rt);

    if (name == "bind") {
        return jsi::Function::createFromHostFunction(
            rt, jsi::PropNameID::forAscii(rt, "bind"), 0,
            [this](jsi::Runtime &rt, const jsi::Value &this_val,
                   const jsi::Value *args, size_t count) -> jsi::Value {
                if (_stmt == nullptr) {
                    throw std::runtime_error("statement has been freed");
                }

                const jsi::Value &js_params = args[0];
                std::vector<JSVariant> params = to_variant_vec(rt, js_params);

                auto promiseCtr =
                    rt.global().getPropertyAsFunction(rt, "Promise");
                auto promise = promiseCtr.callAsConstructor(
                    rt, jsi::Function::createFromHostFunction(
                            rt, jsi::PropNameID::forAscii(rt, "executor"), 0,
                            [this, params](jsi::Runtime &rt,
                                           const jsi::Value &this_val,
                                           const jsi::Value *args,
                                           size_t count) -> jsi::Value {
                                auto resolve =
                                    std::make_shared<jsi::Value>(rt, args[0]);
                                auto reject =
                                    std::make_shared<jsi::Value>(rt, args[1]);
                                auto task = [this, &rt, resolve, reject,
                                             params]() {
                                    try {
#ifdef OP_SQLITE_USE_LIBSQL
                                        opsqlite_libsql_bind_statement(
                                            _stmt, &params);
#else
                                        opsqlite_bind_statement(_stmt,
                                                                &params);
#endif
                                        _js_call_invoker->invokeAsync(
                                            [this, &rt, resolve] {
                                                resolve->asObject(rt)
                                                    .asFunction(rt)
                                                    .call(rt, {});
                                            });
                                    } catch (const std::runtime_error &e) {
                                        _js_call_invoker->invokeAsync(
                                            [this, &rt, e, reject] {
                                                auto errorCtr =
                                                    rt.global()
                                                        .getPropertyAsFunction(
                                                            rt, "Error");
                                                auto error =
                                                    errorCtr
                                                        .callAsConstructor(
                                                            rt,
                                                            jsi::String::
                                                                createFromUtf8(
                                                                    rt,
                                                                    e.what()));
                                                reject->asObject(rt)
                                                    .asFunction(rt)
                                                    .call(rt, error);
                                            });
                                    } catch (const std::exception &e) {
                                        _js_call_invoker->invokeAsync(
                                            [this, &rt, e, reject] {
                                                auto errorCtr =
                                                    rt.global()
                                                        .getPropertyAsFunction(
                                                            rt, "Error");
                                                auto error =
                                                    errorCtr
                                                        .callAsConstructor(
                                                            rt,
                                                            jsi::String::
                                                                createFromUtf8(
                                                                    rt,
                                                                    e.what()));
                                                reject->asObject(rt)
                                                    .asFunction(rt)
                                                    .call(rt, error);
                                            });
                                    }
                                };

                                _thread_pool->queueWork(task);

                                return {};
                            }));
                return promise;
            });
    }

    if (name == "bindSync") {
        return jsi::Function::createFromHostFunction(
            rt, jsi::PropNameID::forAscii(rt, "bindSync"), 0,
            [this](jsi::Runtime &rt, const jsi::Value &this_val,
                   const jsi::Value *args, size_t count) -> jsi::Value {
                if (_stmt == nullptr) {
                    throw std::runtime_error("statement has been freed");
                }

                const jsi::Value &js_params = args[0];
                std::vector<JSVariant> params = to_variant_vec(rt, js_params);
                try {
#ifdef OP_SQLITE_USE_LIBSQL
                    opsqlite_libsql_bind_statement(_stmt, &params);
#else
                    opsqlite_bind_statement(_stmt, &params);
#endif
                } catch (const std::runtime_error &e) {
                    throw std::runtime_error(e.what());
                } catch (const std::exception &e) {
                    throw std::runtime_error(e.what());
                }
                return {};
            });
    }

    if (name == "execute") {
        return jsi::Function::createFromHostFunction(
            rt, jsi::PropNameID::forAscii(rt, "execute"), 0,
            [this](jsi::Runtime &rt, const jsi::Value &this_val,
                   const jsi::Value *args, size_t count) -> jsi::Value {
                if (_stmt == nullptr) {
                    throw std::runtime_error("statement has been freed");
                }

                auto promiseCtr =
                    rt.global().getPropertyAsFunction(rt, "Promise");
                auto promise = promiseCtr.callAsConstructor(
                    rt,
                    jsi::Function::createFromHostFunction(
                        rt, jsi::PropNameID::forAscii(rt, "executor"), 0,
                        [this](jsi::Runtime &rt, const jsi::Value &this_val,
                               const jsi::Value *args,
                               size_t count) -> jsi::Value {
                            auto resolve =
                                std::make_shared<jsi::Value>(rt, args[0]);
                            auto reject =
                                std::make_shared<jsi::Value>(rt, args[1]);

                            auto task = [this, &rt, resolve, reject]() {
                                std::vector<DumbHostObject> results;
                                std::shared_ptr<std::vector<SmartHostObject>>
                                    metadata = std::make_shared<
                                        std::vector<SmartHostObject>>();
                                try {
#ifdef OP_SQLITE_USE_LIBSQL
                                    auto status =
                                        opsqlite_libsql_execute_prepared_statement(
                                            _db, _stmt, &results, metadata);
#else
                                    auto status =
                                        opsqlite_execute_prepared_statement(
                                            _db, _stmt, &results, metadata);
#endif
                                    _js_call_invoker->invokeAsync([&rt,
                                                                   status = std::move(
                                                                       status),
                                                                   results =
                                                                       std::make_shared<
                                                                           std::vector<
                                                                               DumbHostObject>>(
                                                                           results),
                                                                   metadata,
                                                                   resolve] {
                                        auto jsiResult =
                                            create_result(rt, status,
                                                          results.get(),
                                                          metadata);
                                        resolve->asObject(rt)
                                            .asFunction(rt)
                                            .call(rt, std::move(jsiResult));
                                    });
                                } catch (std::exception &exc) {
                                    _js_call_invoker->invokeAsync(
                                        [this, &rt, &exc, reject] {
                                            auto errorCtr =
                                                rt.global()
                                                    .getPropertyAsFunction(
                                                        rt, "Error");
                                            auto error =
                                                errorCtr.callAsConstructor(
                                                    rt,
                                                    jsi::String::
                                                        createFromUtf8(
                                                            rt, exc.what()));
                                            reject->asObject(rt)
                                                .asFunction(rt)
                                                .call(rt, error);
										});
                                }
                            };

                            _thread_pool->queueWork(task);

                            return {};
                        }));

                return promise;
            });
    }

    return {};
}

PreparedStatementHostObject::~PreparedStatementHostObject() {
#ifdef OP_SQLITE_USE_LIBSQL
    if (_stmt != nullptr) {
        libsql_free_stmt(_stmt);
        _stmt = nullptr;
    }
#else
    if (_stmt != nullptr) {
        //    sqlite3_finalize(_stmt);
        _stmt = nullptr;
    }
#endif
}

} // namespace opsqlite