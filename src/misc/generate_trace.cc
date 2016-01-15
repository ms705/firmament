// The Firmament project
// Copyright (c) 2015 Ionel Gog <ionel.gog@cl.cam.ac.uk>
//
// Generate Google style trace.

#include "misc/generate_trace.h"

#include <string>
#include <boost/functional/hash.hpp>

#include "base/common.h"
#include "misc/map-util.h"
#include "misc/utils.h"

DEFINE_bool(generate_trace, false, "Generate Google style trace");
DEFINE_string(generated_trace_path, "",
              "Path to where the trace will be generated");

namespace firmament {

GenerateTrace::GenerateTrace(TimeInterface* time_manager)
  : time_manager_(time_manager) {
  if (FLAGS_generate_trace) {
    MkdirIfNotPresent(FLAGS_generated_trace_path);
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/machine_events");
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/task_events");
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/scheduler_events");
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/task_runtime_events");
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/jobs_num_tasks");
    MkdirIfNotPresent(FLAGS_generated_trace_path + "/task_usage_stat");
    string path =
      FLAGS_generated_trace_path + "machine_events/part-00000-of-00001.csv";
    machine_events_ = fopen(path.c_str(), "w");
    CHECK(machine_events_ != NULL) << "Failed to open: " << path;
    path = FLAGS_generated_trace_path + "scheduler_events/scheduler_events.csv";
    scheduler_events_ = fopen(path.c_str(), "w");
    CHECK(scheduler_events_ != NULL) << "Failed to open: " << path;
    path = FLAGS_generated_trace_path + "task_events/part-00000-of-00500.csv";
    task_events_ = fopen(path.c_str(), "w");
    CHECK(task_events_ != NULL) << "Failed to open: " << path;
    path = FLAGS_generated_trace_path +
      "task_runtime_events/task_runtime_events.csv";
    task_runtime_events_ = fopen(path.c_str(), "w");
    CHECK(task_runtime_events_ != NULL) << "Failed to open: " << path;
    path = FLAGS_generated_trace_path + "jobs_num_tasks/jobs_num_tasks.csv";
    jobs_num_tasks_ = fopen(path.c_str(), "w");
    CHECK(jobs_num_tasks_ != NULL) << "Failed to open: " << path;
    path = FLAGS_generated_trace_path + "task_usage_stat/task_usage_stat.csv";
    task_usage_stat_ = fopen(path.c_str(), "w");
    CHECK(task_usage_stat_ != NULL) << "Failed to open: " << path;
  }
}

GenerateTrace::~GenerateTrace() {
  if (FLAGS_generate_trace) {
    fclose(machine_events_);
    fclose(scheduler_events_);
    fclose(task_events_);
    for (auto& task_id_runtime : task_to_runtime_) {
      uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id_runtime.first);
      TaskRuntime task_runtime = task_id_runtime.second;
      // NOTE: We are using the job id as the job logical name.
      fprintf(task_runtime_events_, "%ju,%ju,%ju,%ju,%ju,%ju,%ju\n",
              *job_id_ptr, task_id_runtime.first, *job_id_ptr,
              task_runtime.start_time, task_runtime.total_runtime,
              task_runtime.runtime, task_runtime.num_runs);
    }
    fclose(task_runtime_events_);
    for (auto& job_to_num_tasks : job_num_tasks_) {
      fprintf(jobs_num_tasks_, "%ju,%ju\n", job_to_num_tasks.first,
              job_to_num_tasks.second);
    }
    fclose(jobs_num_tasks_);
    // TODO(ionel): Collect task usage stats.
    // for (auto& task_to_job : task_to_job_) {
    //   uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_to_job.first);
    //   fprintf(task_usage_stat_, "%ju,%ju,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    //           "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n", *job_id_ptr,
    //           task_to_job.first);
    // }
    fclose(task_usage_stat_);
  }
  // time_manager is not owned by this class. We don't have to delete it here.
}

void GenerateTrace::AddMachine(const ResourceDescriptor& rd) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    // 0 corresponds to add machine.
    int32_t machine_event = 0;

    uint64_t machine_id = GetMachineId(rd);
    fprintf(machine_events_, "%ju,%ju,%d,,,\n",
            timestamp, machine_id, machine_event);
  }
}

uint64_t GenerateTrace::GetMachineId(const ResourceDescriptor& rd) {
  uint64_t machine_id;
  string simulator_machine_prefix = "firmament_simulation_machine_";
  if (rd.has_friendly_name() &&
      rd.friendly_name().find(simulator_machine_prefix) == 0) {
    // This is a simulated machine. Get the trace machine id out of
    // the machine's friendly name.
    string machine_id_str =
      rd.friendly_name().substr(simulator_machine_prefix.size(),
                                string::npos);
    try {
      machine_id = boost::lexical_cast<uint64_t>(machine_id_str);
    } catch (const boost::bad_lexical_cast &) {
      LOG(FATAL) << "Could not convert: " << rd.friendly_name();
    }
  } else {
    size_t hash = 42;
    boost::hash_combine(hash, rd.uuid());
    machine_id = static_cast<uint64_t>(hash);
  }
  return machine_id;
}

void GenerateTrace::RemoveMachine(const ResourceDescriptor& rd) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    // 1 corresponds to remove machine.
    int32_t machine_event = 1;
    uint64_t machine_id = GetMachineId(rd);
    fprintf(machine_events_, "%ju,%ju,%d,,,\n",
            timestamp, machine_id, machine_event);
  }
}

void GenerateTrace::SchedulerRun(
    const scheduler::SchedulerStats& scheduler_stats,
    const DIMACSChangeStats& dimacs_stats) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    fprintf(scheduler_events_, "%ju,%ju,%ju,%ju,%s\n",
            timestamp, scheduler_stats.scheduler_runtime,
            scheduler_stats.algorithm_runtime, scheduler_stats.total_runtime,
            dimacs_stats.GetStatsString().c_str());
  }
}

void GenerateTrace::TaskSubmitted(JobDescriptor* jd_ptr,
                                  TaskDescriptor* td_ptr) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 0;
    uint64_t job_id;
    string simulator_job_prefix = "firmament_simulation_job_";
    TaskID_t task_id = td_ptr->uid();
    uint64_t trace_task_id;
    if (jd_ptr->has_name() &&
        jd_ptr->name().find(simulator_job_prefix) == 0) {
      // The job is coming from a simulation. Get the job id out of
      // the job's name.
      string job_id_str =
        jd_ptr->name().substr(simulator_job_prefix.size(), string::npos);
      try {
        job_id = boost::lexical_cast<uint64_t>(job_id_str);
      } catch (const boost::bad_lexical_cast &) {
        LOG(FATAL) << "Could not convert: " << jd_ptr->name();
      }
      // Set the id to the trace task id which is passed via the index.
      trace_task_id = td_ptr->index();
    } else {
      size_t hash = 42;
      boost::hash_combine(hash, job_id);
      job_id = static_cast<uint64_t>(hash);
      // Not running in simulation mode => set the id to Firmament task id.
      trace_task_id = task_id;
    }
    // We use the Firmament task id here because the other methods in this
    // class need to access the collection. They only get called with the
    // Firmament task id.
    bool inserted = InsertIfNotPresent(&task_to_job_, task_id, job_id);
    if (inserted) {
      uint64_t* num_tasks = FindOrNull(job_num_tasks_, job_id);
      if (num_tasks == NULL) {
        InsertIfNotPresent(&job_num_tasks_, job_id, 1);
      } else {
        *num_tasks = *num_tasks + 1;
      }
    }
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, job_id, trace_task_id, task_event);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    if (tr_ptr == NULL) {
      TaskRuntime task_runtime;
      task_runtime.task_id = trace_task_id;
      task_runtime.start_time = timestamp;
      task_runtime.num_runs = 0;
      task_runtime.last_schedule_time = 0;
      InsertIfNotPresent(&task_to_runtime_, task_id, task_runtime);
    }
  }
}

void GenerateTrace::TaskCompleted(TaskID_t task_id) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 4;
    uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id);
    CHECK_NOTNULL(job_id_ptr);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    CHECK_NOTNULL(tr_ptr);
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, *job_id_ptr, tr_ptr->task_id, task_event);
    // XXX(ionel): This assumes that only one task with task_id is running
    // at a time.
    tr_ptr->total_runtime += timestamp - tr_ptr->last_schedule_time;
    tr_ptr->runtime = timestamp - tr_ptr->last_schedule_time;
  }
}

void GenerateTrace::TaskEvicted(TaskID_t task_id) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 2;
    uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id);
    CHECK_NOTNULL(job_id_ptr);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    CHECK_NOTNULL(tr_ptr);
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, *job_id_ptr, tr_ptr->task_id, task_event);
    // XXX(ionel): This assumes that only one task with task_id is running
    // at a time.
    tr_ptr->total_runtime += timestamp - tr_ptr->last_schedule_time;
  }
}

void GenerateTrace::TaskFailed(TaskID_t task_id) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 3;
    uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id);
    CHECK_NOTNULL(job_id_ptr);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    CHECK_NOTNULL(tr_ptr);
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, *job_id_ptr, tr_ptr->task_id, task_event);
    // XXX(ionel): This assumes that only one task with task_id is running
    // at a time.
    tr_ptr->total_runtime += timestamp - tr_ptr->last_schedule_time;
  }
}

void GenerateTrace::TaskKilled(TaskID_t task_id) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 5;
    uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id);
    CHECK_NOTNULL(job_id_ptr);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    CHECK_NOTNULL(tr_ptr);
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, *job_id_ptr, tr_ptr->task_id, task_event);
    // XXX(ionel): This assumes that only one task with task_id is running
    // at a time.
    tr_ptr->total_runtime += timestamp - tr_ptr->last_schedule_time;
  }
}

void GenerateTrace::TaskScheduled(TaskID_t task_id, ResourceID_t res_id) {
  if (FLAGS_generate_trace) {
    uint64_t timestamp = time_manager_->GetCurrentTimestamp();
    int32_t task_event = 1;
    uint64_t* job_id_ptr = FindOrNull(task_to_job_, task_id);
    CHECK_NOTNULL(job_id_ptr);
    TaskRuntime* tr_ptr = FindOrNull(task_to_runtime_, task_id);
    CHECK_NOTNULL(tr_ptr);
    fprintf(task_events_, "%ju,,%ju,%ju,%d,,,,,,,\n",
            timestamp, *job_id_ptr, tr_ptr->task_id, task_event);
    tr_ptr->num_runs++;
    tr_ptr->last_schedule_time = timestamp;
  }
}

} // namespace firmament
