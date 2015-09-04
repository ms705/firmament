// The Firmament project
// Copyright (c) 2015 Ionel Gog <ionel.gog@cl.cam.ac.uk>

#ifndef FIRMAMENT_SIM_TRACE_EXTRACT_KNOWLEDGE_BASE_SIMULATOR_H
#define FIRMAMENT_SIM_TRACE_EXTRACT_KNOWLEDGE_BASE_SIMULATOR_H

#include "scheduling/knowledge_base.h"

#include "sim/trace-extract/google_trace_utils.h"

namespace firmament {
namespace sim {

class KnowledgeBaseSimulator : public KnowledgeBase {
 public:
  KnowledgeBaseSimulator();
  void AddMachineSample(
      uint64_t current_time,
      ResourceDescriptor* rd_ptr,
      const unordered_map<TaskID_t, ResourceDescriptor*>& task_id_to_rd);
  void EraseStats(TaskID_t task_id);
  void SetTaskStats(TaskID_t task_id, const TaskStats& task_stat);

 private:
  unordered_map<TaskID_t, TaskStats> task_stats_;
};

} // namespace sim
} // namespace firmament

#endif  // FIRMAMENT_SIM_TRACE_EXTRACT_KNOWLEDGE_BASE_SIMULATOR_H
