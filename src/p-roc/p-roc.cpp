#ifdef PROC_SUPPORT

#include <stdarg.h>

extern "C" {
#include "driver.h"
#include "wpc/core.h"
#include "wpc/sim.h"
}

#include <fstream>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <p-roc/pinproc.h>
#include "p-roc.h"
#include "p-roc_drivers.h"

// Handle to the P-ROC instance.
PRHandle proc = NULL;

// Create a global yamlDoc to hold the machine data parsed from the YAML file.
// Other p-roc support files need access to it. No sense passing it around
// everywhere.
YAML::Node yamlDoc;

// Global procType is used in many P-ROC functions to do different things
// based on the type of machine being used.
PRMachineType procType;

// Load/Parse the YAML file.
PRMachineType procLoadMachineYAML(char *filename) {
	PRMachineType machineType = kPRMachineInvalid;

	try	{
		std::ifstream fin(filename);
		if (fin.is_open() == false) {
			fprintf(stderr, "YAML file not found: %s\n", filename);
			return kPRMachineInvalid;
		}
		YAML::Parser parser(fin);

		while(parser) {
				parser.GetNextDocument(yamlDoc);
		}
		std::string machineTypeString;
		yamlDoc["PRGame"]["machineType"] >> machineTypeString;
		if (machineTypeString == "wpc") {
			machineType = kPRMachineWPC;
			fprintf(stderr, "YAML machine type: kPRMachineWPC\n");
		} else if (machineTypeString == "wpc95") {
			machineType = kPRMachineWPC95;
			fprintf(stderr, "YAML machine type: kPRMachineWPC95\n");
		} else if (machineTypeString == "wpcAlphanumeric") {
			machineType = kPRMachineWPCAlphanumeric;
			fprintf(stderr, "YAML machine type: kPRMachineWPCAlphanumeric\n");
		} else if(machineTypeString == "sternWhitestar") {
			machineType = kPRMachineSternWhitestar;
			fprintf(stderr, "YAML machine type: kPRMachineSternWhitestar\n");
		} else if(machineTypeString == "sternSAM") {
			machineType = kPRMachineSternSAM;
			fprintf(stderr, "YAML machine type: kPRMachineSternSAM\n");
		} else {
			fprintf(stderr, "Unknown machine type in YAML file: %s\n", machineTypeString.c_str());
			return kPRMachineInvalid;
		}
	}
	catch (...)	{
		fprintf(stderr, "Unexpected exception while parsing YAML config.\n");
	}

	return machineType;
}

// Set the machine type.
PRMachineType procSetMachineType(char *yaml_filename) {
	// First set the machine type based on the ROM being run.
	switch (core_gameData->gen) {
		case GEN_WPCALPHA_1:
		case GEN_WPCALPHA_2:
			if (pmoptions.alpha_on_dmd) {
				procType = kPRMachineWPC;
				fprintf(stderr, "ROM machine type: kPRMachineWPCAlphanumeric,\nbut using kPRMachineWPC due to alpha_on_dmd option\n");
			} else {
				procType = kPRMachineWPCAlphanumeric;
				fprintf(stderr, "ROM machine type: kPRMachineWPCAlphanumeric\n");
			}
			break;
		case GEN_WPCDMD:
		case GEN_WPCFLIPTRON:
		case GEN_WPCDCS:
		case GEN_WPCSECURITY:
			procType = kPRMachineWPC;
			fprintf(stderr, "ROM machine type: kPRMachineWPC\n");
			break;
		case GEN_WPC95DCS:
		case GEN_WPC95:
			procType = kPRMachineWPC95;
			fprintf(stderr, "ROM machine type: kPRMachineWPC95\n");
			break;
		case GEN_WS:
		case GEN_WS_1:
		case GEN_WS_2:
			procType = kPRMachineSternWhitestar;
			fprintf(stderr, "ROM machine type: kPRMachineSternWhitestar\n");
			break;
		case GEN_SAM:
			procType = kPRMachineSternSAM;
			fprintf(stderr, "ROM machine type kPRMachineSternSAM\n");
			break;
		default:
			procType = kPRMachineInvalid;
			fprintf(stderr, "Unknown ROM machine type in YAML file\n");
	}

	// Now get the machine type identified in the YAML file and
	// compare it to the ROM machine type. If not the same, there's
	// a problem.
	PRMachineType yamlMachineType = kPRMachineInvalid;
	if (strcmp(yaml_filename, "None") == 0) {
		return procType;
	} else {
		std::ifstream fin(yaml_filename);
		if (fin.is_open() == false) {
			fprintf(stderr, "YAML file not found: %s\n", yaml_filename);
			return kPRMachineInvalid;
		} else {
			yamlMachineType = procLoadMachineYAML(yaml_filename);
			if (yamlMachineType != procType) {
				fprintf(stderr, "Machine type in YAML does not match the machine type of the ROM.\n");
				return kPRMachineInvalid;
			} else {
				return procType;
			}
		}
	}
}

// Send all pending commands to the P-ROC.
void procFlush(void) {
	PRFlushWriteData(proc);
}

void procDeinitialize() {
	if (proc) PRDelete(proc);
}

// Initialize the P-ROC hardware.
int procInitialize(char *yaml_filename) {
	fprintf(stderr, "\n\n****** Initializing P-ROC ******\n");

	procType = procSetMachineType(yaml_filename);
	if (procType != kPRMachineInvalid) {
		proc = PRCreate(procType);
		if (proc == kPRHandleInvalid) {
			fprintf(stderr, "Error during PRCreate: %s\n", PRGetLastErrorText());
			fprintf(stderr, "\n****** Ending P-ROC Initialization ******\n");
			return 0;
		} else {
// TODO/PROC: Change PinMAME to always have these variables
#ifdef VPINMAME
			g_fHandleKeyboard = 0;
			g_fHandleMechanics = 0;
#endif
			PRReset(proc, kPRResetFlagUpdateDevice);
			procConfigureDefaultSwitchRules();

			procInitializeCoilDrivers();
			procSetSwitchStates();
			procConfigureSwitchRules();
			procConfigureDriverDefaults();

			if (procType != kPRMachineWPCAlphanumeric) {
				procDMDInit();
			}

			fprintf(stderr, "\n****** P-ROC Initialization COMPLETE ******\n\n");
		}
	}

	return (procType != kPRMachineInvalid);
}

// Tickle the P-ROC's watchdog so it doesn't disable driver outputs.
void procTickleWatchdog(void) {
	PRDriverWatchdogTickle(proc);
}

// The following is a work around for using MinGW with gcc 3.2.3 to compile
// the yaml-cpp dependency.  gcc 3.2.3 is missing the definition of 'strtold'
// in libstdc++, and yaml-cpp makes heavy use of stringstream, which uses
// strtold internally.  Defining strtold here allows pinmame to link with
// yaml-cpp, and by using strtod, it will work properly for everything except
// longs, which shouldn't be used in pinball YAML files anyway.

#if (__MINGW32__) && (__GNUC__) && (__GNUC__ < 4)
long double strtold(const char *__restrict__ nptr, char **__restrict__ endptr) {
	return strtod(nptr, endptr);
}
#endif

#endif /* PROC_SUPPORT */
