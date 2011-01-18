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

// Global procType is used in many P-ROC functions to do different things
// based on the type of machine being used.
PRMachineType machineType;

// Create a global yamlDoc to hold the machine data parsed from the YAML file.
// Other p-roc support files need access to it. No sense passing it around
// everywhere.
YAML::Node yamlDoc;

// Load/Parse the YAML file.
PRMachineType procLoadMachineYAML(char *filename) {
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
PRMachineType getRomMachineType() {
	// First set the machine type based on the ROM being run.
	switch (core_gameData->gen) {
		case GEN_WPCALPHA_1:
		case GEN_WPCALPHA_2:
			if (pmoptions.alpha_on_dmd) {
				return kPRMachineWPC;
				fprintf(stderr, "ROM machine type: kPRMachineWPCAlphanumeric,\nbut using kPRMachineWPC due to alpha_on_dmd option\n");
			} else {
				return kPRMachineWPCAlphanumeric;
				fprintf(stderr, "ROM machine type: kPRMachineWPCAlphanumeric\n");
			}
			break;
		case GEN_WPCDMD:
		case GEN_WPCFLIPTRON:
		case GEN_WPCDCS:
		case GEN_WPCSECURITY:
			return kPRMachineWPC;
			fprintf(stderr, "ROM machine type: kPRMachineWPC\n");
			break;
		case GEN_WPC95DCS:
		case GEN_WPC95:
			return kPRMachineWPC95;
			fprintf(stderr, "ROM machine type: kPRMachineWPC95\n");
			break;
		case GEN_WS:
		case GEN_WS_1:
		case GEN_WS_2:
			return kPRMachineSternWhitestar;
			fprintf(stderr, "ROM machine type: kPRMachineSternWhitestar\n");
			break;
		case GEN_SAM:
			return kPRMachineSternSAM;
			fprintf(stderr, "ROM machine type kPRMachineSternSAM\n");
			break;
		default:
			return kPRMachineInvalid;
			fprintf(stderr, "Unknown ROM machine type in YAML file\n");
	}
}

void setMachineType(char *yaml_filename) {
	if (strcmp(yaml_filename, "None") == 0) {
		machineType = kPRMachineInvalid;
	} else {
		std::ifstream fin(yaml_filename);
		if (fin.is_open() == false) {
			fprintf(stderr, "YAML file not found: %s\n", yaml_filename);
			machineType = kPRMachineInvalid;
		} else {
			// TODO: Make sure the machineType field exists in file
			machineType = procLoadMachineYAML(yaml_filename);
		}
	}
}

// Send all pending commands to the P-ROC.
void procFlush(void) {
	PRFlushWriteData(proc);
}

void procDeinitialize() {
	if (proc) {
		PRDelete(proc);
	}
}

// Initialize the P-ROC hardware.
int procInitialize(char *yaml_filename) {
	fprintf(stderr, "\n\n****** Initializing P-ROC ******\n");

	setMachineType(yaml_filename);
	if (machineType != kPRMachineInvalid) {
		proc = PRCreate(machineType);
		if (proc == kPRHandleInvalid) {
			fprintf(stderr, "Error during PRCreate: %s\n", PRGetLastErrorText());
			fprintf(stderr, "\n****** Ending P-ROC Initialization ******\n");
			return 0;
		} else {
			PRReset(proc, kPRResetFlagUpdateDevice);
			procConfigureDefaultSwitchRules();
			procConfigureInputMap();

			procInitializeCoilDrivers();
			procConfigureSwitchRules();
			procConfigureDriverDefaults();

			if (machineType != kPRMachineWPCAlphanumeric) {
				procDMDInit();
			}

			fprintf(stderr, "\n****** P-ROC Initialization COMPLETE ******\n\n");
		}
	}

	if (machineType != kPRMachineInvalid) {
		return 1;
	} else {
		return 0;
	}
}

int procIsActive(void) {

	PRMachineType romMachineType = getRomMachineType();

	// Now compare machine types. If not the same, there's
	// a problem.
	if (proc) {
		if (machineType != romMachineType) {
			fprintf(stderr, "Machine type in YAML does not match the machine type of the ROM.\n");
			return 0;
		} else {
			procSetSwitchStates();
			return 1;
		}
	} else {
		return 0;
	}
}

// Tickle the P-ROC's watchdog so it doesn't disable driver outputs.
void procTickleWatchdog(void) {
	PRDriverWatchdogTickle(proc);
}

// The following is a work around for using MinGW with gcc 3.2.3 to compile
// the yaml-cpp dependency. gcc 3.2.3 is missing the definition of 'strtold'
// in libstdc++, and yaml-cpp makes heavy use of stringstream, which uses
// strtold internally. Defining strtold here allows pinmame to link with
// yaml-cpp, and by using strtod, it will work properly for everything except
// longs, which shouldn't be used in pinball YAML files anyway.

#if (__MINGW32__) && (__GNUC__) && (__GNUC__ < 4)
long double strtold(const char *__restrict__ nptr, char **__restrict__ endptr) {
	return strtod(nptr, endptr);
}
#endif

#endif /* PROC_SUPPORT */
