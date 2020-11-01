#include "app_cli.h"

app_cli_status_e cli__crash_me(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output);

app_cli_status_e cli__i2c(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                          app_cli__print_string_function cli_output);

app_cli_status_e cli__task_list(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                app_cli__print_string_function cli_output);

/* --------------------------------- LAB CLI -------------------------------- */

/* -------------------------------- parameter ------------------------------- */
/*
 * argument =    NULL
 * -->app_cli__argument_t ( This is not utilize in SJ Board ) =  NULL
 *
 * user_input_minus_command_name =      UserInput
 * -->sl_string_t ( Powerful String library ) Process command "sl_string.h"
 *
 * cli_ouput =      Function pointer that output backdata to CLI
 * --> app_cli__print_string_function
 */
app_cli_status_e cli__bang_handler(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                   app_cli__print_string_function cli_output);

/* -------------------------------------------------------------------------- */

/* --------------------------------- mp3 CLI -------------------------------- */
typedef char trackname_t[64];
app_cli_status_e cli__mp3_play(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output);
