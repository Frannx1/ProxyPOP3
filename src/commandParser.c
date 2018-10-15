/**
 * commandParser.c -- parser for POPV3 commands
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "commandParser.h"
#include "errorslib.h"
#include "string.h"


#define MAX_MSG_SIZE 512

typedef struct commandProps {
    commandType type;
    char *      name;
    size_t      length;
    size_t      argsQtyMin;
    size_t      argsQtyMax;
} commandProps;

#define IS_MULTILINE(command) ((command.type == CMD_LIST && command.argsQty == 0) \
                             || command.type == CMD_RETR || command.type == CMD_CAPA)


static const commandProps commandTable[] = {
    {
        .type = CMD_USER, .name = "USER", .length = 4, .argsQtyMin = 1, .argsQtyMax = 512 - 7,  //un user puede contener espacios
    } , {
        .type = CMD_PASS, .name = "PASS", .length = 4, .argsQtyMin = 1, .argsQtyMax = 512 - 7,
    } , {        
        .type = CMD_APOP, .name = "APOP", .length = 4, .argsQtyMin = 2, .argsQtyMax = 512 - 7,
    } , {
        .type = CMD_RETR, .name = "RETR", .length = 4, .argsQtyMin = 1, .argsQtyMax = 1,
    } , {
        .type = CMD_LIST, .name = "LIST", .length = 4, .argsQtyMin = 0, .argsQtyMax = 1,
    } , {
        .type = CMD_CAPA, .name = "CAPA", .length = 4, .argsQtyMin = 0, .argsQtyMax = 512 - 7,
    }
};

static const char * crlfMsg     = "\r\n";
static const int    crlfMsgSize = 2;


static initializeCommand(commandStruct * command, const uint8_t * ptr, bool * invalidType) {
    command->type = CMD_OTHER;
    command->argsQty = 0;
    command->indicator = false;
    command->startCommandPtr = ptr;
    command->startResponsePtr = NULL;
    command->responseSize = -1;
    for(int i = 0; i < CMD_TYPES_QTY; i++) 
        invalidType[i] = false;
}

void commandParserInit(commandParser * parser) {
    parser->state     = COMMAND_TYPE;
    parser->lineSize  = 0;
    parser->stateSize = 0;
}
//checkear commandSize
commandState commandParserFeed(commandParser * parser, const uint8_t * ptr, commandStruct * commands, size_t * commandsSize) {
    if(parser->lineSize == 0)
        initializeCommand(&commands[*commandsSize], ptr, parser->invalidType);

    commandStruct currentCommand = commands[*commandsSize];
    const uint8_t c = *ptr;

    switch(parser->state) {
        case COMMAND_TYPE:
            for(size_t i = 0; i < CMD_TYPES_QTY; i++) {
                if(!parser->invalidType[i]) {
                    if(toupper(c) != commandTable[i].name[parser->lineSize]) {
                        parser->invalidType[i] = true;
                        parser->invalidQty++;
                    }
                    else if(parser->lineSize == commandTable[i].length - 1) {
                        currentCommand.type = commandTable[i].type;
                        parser->stateSize = 0;
                        if(commandTable[i].argsQtyMax > 0)
                            parser->state = COMMAND_ARGS;
                        else
                            parser->state = COMMAND_CRLF;
                        break;
                    }
                }
                if(parser->invalidQty == CMD_TYPES_QTY)
                    parser->state = COMMAND_ERROR;
            }
            break;

        case COMMAND_ARGS:
            if(c == ' ') {
                if(currentCommand.argsQty == commandTable[currentCommand.type].argsQtyMax)
                    parser->state = COMMAND_ERROR;
                else if(parser->stateSize == 0) 
                    parser->stateSize++;
                else if(parser->stateSize > 1 && currentCommand.argsQty < commandTable[currentCommand.type].argsQtyMax) {
                    parser->stateSize = 1;
                    currentCommand.argsQty++;
                }
            } else if(c != crlfMsg[0] && c != crlfMsg[1]) {
                if(parser->stateSize == 0)
                    parser->state = COMMAND_ERROR;
                else 
                    parser->stateSize++;
            } else if(c == crlfMsg[0] && commandTable[currentCommand.type].argsQtyMin <= ++currentCommand.argsQty && currentCommand.argsQty <= commandTable[currentCommand.type].argsQtyMax) {
                parser->state     = COMMAND_CRLF;
                parser->stateSize = 1;
            } else
                parser->state     = COMMAND_ERROR;
            break;

        case COMMAND_CRLF:
            if(c == crlfMsg[parser->stateSize]) {
                if(parser->stateSize == crlfMsgSize - 1) {
                    parser->state     = COMMAND_TYPE;
                    parser->lineSize  = 0;
                    parser->stateSize = 0;
                    currentCommand.isMultiline = IS_MULTILINE(currentCommand);
                    *commandsSize++;
                }
            }
            else
                parser->state = COMMAND_ERROR;
            break;

        case COMMAND_ERROR:
            if(c == crlfMsg[0]) {
                currentCommand.type = CMD_OTHER;
                parser->state     = COMMAND_CRLF;
                parser->stateSize = 1;
            }
            break;
        default:
            fail("COMMAND parser not reconize state: %d", parser->state);
    }
    if(parser->lineSize++ == MAX_MSG_SIZE)
        parser->state = COMMAND_ERROR;
    return parser->state;
}

commandState commandConsume(commandParser * parser, bufferADT buffer, commandStruct * commands, size_t * commandsSize) {
    commandState state = parser->state;
    size_t bufferSize;
    uint8_t * ptr = getReadPtr(buffer, &bufferSize);   

    for(size_t i = 0; i < bufferSize; i++) {
        state = commandParserFeed(parser, &ptr[i], commands, commandsSize);
    }
    return state;
}

char * getUsername(const commandStruct command) {
    char * ptr = command.startCommandPtr + commandTable[command.type].length + 1;
    return copyToNewStringEndedIn(ptr, '\r');
}

/*
char * getArg(const commandStruct command, size_t argNo) {
    if(argNo == 0 || argNo > command.argsQty)
        return NULL;

    int currentArg = 0;
    char * ptr = command.startCommandPtr;

    while(currentArg < argNo) {
        while(*ptr++ != ' ' && *ptr != '\r' && *ptr != '\n');
        if(*ptr == '\r' || *ptr == '\n')
            return NULL;
        currentArg++;
    }
}
*/
