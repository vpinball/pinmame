/*
  Event.h
  Created by Markus Kalkbrenner, 2021-2022.

  Play more pinball!
*/

#ifndef EVENT_h
#define EVENT_h

#define PLATFORM_WPC           1
#define PLATFORM_DATA_EAST     2
#define PLATFORM_SYS11         3

#define EVENT_SOURCE_ANY      42 // "*"
#define EVENT_SOURCE_DEBUG    66 // "B" Debug
#define EVENT_CONFIGURATION   67 // "C" Configure I/O
#define EVENT_SOURCE_DMD      68 // "D" VPX/DOF/PUP
#define EVENT_SOURCE_EVENT    69 // "E" VPX/DOF/PUP common event from different system, like
#define EVENT_SOURCE_EFFECT   70 // "F" custom event from running Effect
#define EVENT_SOURCE_GI       71 // "G" WPC GI
#define EVENT_SOURCE_LIGHT    76 // "L" VPX/DOF/PUP lights, mainly playfield inserts
#define EVENT_NULL            78 // "N" NULL event
#define EVENT_SOURCE_SOUND    79 // "O" sound command
#define EVENT_POLL_EVENTS     80 // "P" Poll events command, mainly read switches
#define EVENT_READ_SWITCHES   82 // "R" Read current state of all switches on i/o boards
#define EVENT_SOURCE_SOLENOID 83 // "S" VPX/DOF/PUP includes flashers
#define EVENT_SOURCE_SWITCH   87 // "W" VPX/DOF/PUP

#define CONFIG_TOPIC_COILS    99  // "c"
#define CONFIG_TOPIC_FLASHERS 102 // "f"
#define CONFIG_TOPIC_LAMPS    108 // "l"
#define CONFIG_TOPIC_MECHS    109 // "m"
#define CONFIG_TOPIC_SWITCHES 115 // "s"

#define CONFIG_TOPIC_SWITCHES_NUMBER 78 // "N"
#define CONFIG_TOPIC_SWITCHES_PORT   80 // "P"

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

struct Event {
    UINT8 sourceId;
    UINT16 eventId;
    UINT8 value;

    Event(UINT8 sId, UINT16 eId) {
        sourceId = sId;
        eventId = eId;
        value = 1;
    }

    Event(UINT8 sId, UINT16 eId, UINT8 v) {
        sourceId = sId;
        eventId = eId;
        value = v;
    }

    bool operator==(const Event &other) const {
        return this->sourceId == other.sourceId
            && this->eventId == other.eventId
            && this->value == other.value;
    }

    bool operator!=(const Event &other) const {
        return !(*this == other);
    }
};

struct ConfigEvent {
    UINT8 sourceId; // EVENT_CONFIGURATION
    UINT8 boardId;  //
    UINT8 topic;    // lamps
    UINT8 index;    // 0, index of assignment
    UINT8 key;      // ledType, assignment/brightness
    UINT32 value;   // FFFF00FF

    ConfigEvent(UINT8 b, UINT8 t, UINT8 i, UINT8 k, UINT32 v) {
        sourceId = EVENT_CONFIGURATION;
        boardId = b;
        topic = t;
        index = i;
        key = k;
        value = v;
    }
};

#endif
