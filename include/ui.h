#pragma once
// Minimal UI header for OptiLoud56
// Only declares the property builder that OBS calls from filter_autoloudness.cpp

#ifdef __cplusplus
extern "C" {
#endif

struct AutoParams;               // forward-declare (definition lives in filter_autoloudness.h)

struct obs_properties;           // forward declarations to avoid heavy includes
typedef struct obs_properties obs_properties_t;

// Build and return the properties for the filter UI.
// Safe to call from C++ translation units.
obs_properties_t* optiloud56_build_properties(const AutoParams* init);

#ifdef __cplusplus
} // extern "C"
#endif