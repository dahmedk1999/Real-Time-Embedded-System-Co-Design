#include "cli_handlers.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "string.h"
#include <stdio.h>

/* ----------------------------- Task List Print ---------------------------- */

static void cli__task_list_print(sl_string_t user_input_minus_command_name, app_cli__print_string_function cli_output);

/* -------------------------------- Crash Me -------------------------------- */

app_cli_status_e cli__crash_me(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output) {
  uint32_t *bad_pointer = (uint32_t *)0x00000001;
  *bad_pointer = 0xDEADBEEF;
  return APP_CLI_STATUS__SUCCESS;
}

/* -------------------------------- Task List ------------------------------- */

app_cli_status_e cli__task_list(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                app_cli__print_string_function cli_output) {
  const int sleep_time = sl_string__to_int(user_input_minus_command_name);
  if (sleep_time > 0) {
    vTaskResetRunTimeStats();
    vTaskDelay(sleep_time);
  }

  // re-use user_input_minus_command_name as 'output_string' to save memory:
  sl_string_t output_string = user_input_minus_command_name;
  cli__task_list_print(output_string, cli_output);

  return APP_CLI_STATUS__SUCCESS;
}

/* -------------------------------------------------------------------------- */

static void cli__task_list_print(sl_string_t output_string, app_cli__print_string_function cli_output) {
  void *unused_cli_param = NULL;

#if (0 != configUSE_TRACE_FACILITY)
  // Enum to char : eRunning, eReady, eBlocked, eSuspended, eDeleted
  static const char *const task_status_table[] = {"running", " ready ", "blocked", "suspend", "deleted"};

  // Limit the tasks to avoid heap allocation.
  const unsigned portBASE_TYPE max_tasks = 10;
  TaskStatus_t status[max_tasks];
  uint32_t total_cpu_runtime = 0;
  uint32_t total_tasks_runtime = 0;

  const uint32_t total_run_time = portGET_RUN_TIME_COUNTER_VALUE();
  const unsigned portBASE_TYPE task_count = uxTaskGetSystemState(&status[0], max_tasks, &total_cpu_runtime);

  sl_string__printf(output_string, "%10s  Status Pr Stack CPU%%          Time\n", "Name");
  cli_output(unused_cli_param, output_string);

  for (unsigned priority_number = 0; priority_number < configMAX_PRIORITIES; priority_number++) {
    /* Print in sorted priority order */
    for (unsigned i = 0; i < task_count; i++) {
      const TaskStatus_t *task = &status[i];
      if (task->uxBasePriority == priority_number) {
        total_tasks_runtime += task->ulRunTimeCounter;

        const unsigned cpu_percent = (0 == total_cpu_runtime) ? 0 : task->ulRunTimeCounter / (total_cpu_runtime / 100);
        const unsigned time_us = task->ulRunTimeCounter;
        const unsigned stack_in_bytes = (sizeof(void *) * task->usStackHighWaterMark);

        sl_string__printf(output_string, "%10s %s %2u %5u %4u %10u us\n", task->pcTaskName,
                          task_status_table[task->eCurrentState], (unsigned)task->uxBasePriority, stack_in_bytes,
                          cpu_percent, time_us);
        cli_output(unused_cli_param, output_string);
      }
    }
  }

  sl_string__printf(output_string, "Overhead: %u uS\n", (unsigned)(total_run_time - total_tasks_runtime));
  cli_output(unused_cli_param, output_string);
#else
  cli_output(unused_cli_param, "Unable to provide you the task information along with their CPU and stack usage.\n");
  cli_output(unused_cli_param, "configUSE_TRACE_FACILITY macro at FreeRTOSConfig.h must be non-zero\n");
#endif
}
/* -------------------------------------------------------------------------- */
/* ---------------------------- CLI Bang Handler ---------------------------- */
app_cli_status_e cli__bang_handler(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                   app_cli__print_string_function cli_output) {
  sl_string_t s = user_input_minus_command_name;

  /* ------------------------------- EXPLANATION ------------------------------ */
  /*******************************************************************************
   *     If the user types 'taskcontrol suspend led0'
   *     --> then we need to suspend a task with the name of 'led0'
   *     In this case, the user_input_minus_command_name will be set to 'suspend led0'
   *     --> with the command-name is removed
   ********************************************************************************/

  /* --------------------------- STRING MANIPULATION -------------------------- */
  /*******************************************************************************
   *   we cannot use 'sl_string__printf("Failed to find %s", s);
   *   --->because that would print existing string onto
   *   ---> Use: cli_output(NULL, s); to printout object
   *   In this case, try to query the tasks with the name 'led0'
   *   ---> Use: bool sl_string__erase_first_word(sl_string_t string, char word_separater)
   *   ->OR Use: char name[16]; sl_string__scanf("%*s %16s", name);
   *********************************************************************************/
  if (sl_string__begins_with_ignore_case(s, "suspend")) // SUSPEND
  {
    /* Erase first word of string until space */
    sl_string__erase_first_word(s, ' ');
    /* Assign object S --> task_handle */
    TaskHandle_t task_handle = xTaskGetHandle(s);

    /* Wrong Task_name handler */
    if (NULL == task_handle) {
      sl_string__insert_at(s, 0, "Could not find a task with name:");
      cli_output(NULL, s);
    }
    vTaskSuspend(task_handle);
  } else if (sl_string__begins_with_ignore_case(s, "resume")) // RESUME
  {
    /* Erase first word of string until space */
    sl_string__erase_first_word(s, ' ');
    /* Assign object S --> task_handle */
    TaskHandle_t task_handle = xTaskGetHandle(s);

    /* Wrong Task_name handler */
    if (NULL == task_handle) {
      sl_string__insert_at(s, 0, "Could not find a task with name:");
      cli_output(NULL, s);
    }
    vTaskResume(task_handle);
  } else // WITHOUT ACTION Statement
  {
    cli_output(NULL, "Did you mean to say suspend or resume?\n");
  }

  return APP_CLI_STATUS__SUCCESS;
}

extern QueueHandle_t Q_trackname;
app_cli_status_e cli__mp3_play(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output) {
  // user_input_minus_command_name is actually a 'char *' pointer type
  // We tell the Queue to copy 32 bytes of songname from this location
  trackname_t trackname;
  memset(trackname, 0, sizeof(trackname));

  if (sl_string__get_length(user_input_minus_command_name) < 63) {
    strncpy(trackname, user_input_minus_command_name, 64);
    xQueueSend(Q_trackname, trackname, portMAX_DELAY);
    printf("Sent %s over to the Q_songname\n", user_input_minus_command_name);
  } else {
    printf("Error: please enter in a song name less than 64 characters \n");
  }
  return APP_CLI_STATUS__SUCCESS;
}