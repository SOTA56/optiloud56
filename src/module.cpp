
#include <obs-module.h>

extern "C" struct obs_source_info optiloud56_filter_info;

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("SOTA56")
OBS_MODULE_USE_DEFAULT_LOCALE("OptiLoud56", "data/locale")

bool obs_module_load(void)
{
  obs_register_source(&optiloud56_filter_info);
  blog(LOG_INFO, "[OptiLoud56] filter registered (ButtonUI build)");
  return true;
}
