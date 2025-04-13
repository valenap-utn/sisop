#include <cpu_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;

void inicializarCpu(){

    config = config_create("./cpu.config");
    levantarConfig();

    logger = log_create("cpu.log", "CPU", 1, current_log_level);

}
void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}