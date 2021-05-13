#pragma once

#include <bvar/bvar.h>
#include <gflags/gflags.h>
#include <memory>

#include "normalization_message.pb.h"
#include "util.pb.h"

#define MODEL_RESULT_PASS "pass"
#define MODEL_RESULT_FAIL "fail"
#define MODEL_RESULT_REVIEW "review"
#define MODEL_RESULT_BLOCK "block"

DECLARE_string(audit_type);

namespace bigoai {

using ContextPtr = std::shared_ptr<Context>;

void log_context_normalization_msg(ContextPtr &ctx);

bool context_data_check(ContextPtr &ctx);

size_t get_context_data_size(ContextPtr &ctx);

const std::string get_context_url(ContextPtr &ctx);

AuditDataSubType *get_context_data_object(ContextPtr &ctx);
/*
  see proto/util.proto/AuditData
*/
AuditDataSubType *get_context_data_object_by_type(ContextPtr &ctx, const std::string type);

void set_context_detailmlresult_modelinfo(ContextPtr &ctx, const std::string &key, const std::string result,
                                          const std::string info);
void set_context_detailmlresult_modelinfo(ContextPtr &ctx, const std::string &key, const std::string result,
                                          const std::string info, const std::string &type);

void set_context_detailmlresult_modelinfo_with_type(ContextPtr &ctx, const std::string &key, const std::string result,
                                                    const std::string info, const std::string type);

void set_context_modelinfo(ContextPtr &ctx, const std::string &key, const std::string info);

void set_context_timestamp(ContextPtr &ctx, const std::string &key);

bool context_is_download_failed(ContextPtr &ctx);

void discard_context(ContextPtr &ctx);
void discard_context_without_log(ContextPtr &ctx);

}
