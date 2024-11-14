/*
 * flagControl
 *   Copyright (C) 2023 GEP, zeL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include "bzfsAPI.h"
#include "plugin_files.h"
#define BUFFLEN 256

class flagControl : public bz_Plugin, public bz_CustomSlashCommandHandler
{
public:
    virtual const char* Name();
    virtual void Init(const char* config);
    virtual void Cleanup();
    virtual void Event(bz_EventData* eventData);
    virtual bool SlashCommand(int playerID, bz_ApiString command, bz_ApiString /*message*/, bz_APIStringList *params);

private:
    virtual void LoadControlledFlags(const char* commandline);
    virtual void UpdateState(void);
    virtual void ReloadControlledFlagsFile();
    virtual std::pair<std::string, int> SplitFlag(std::string);

    bool allowFC;
    bool currentlyFC;
    int playerCount;

    std::vector<std::pair<std::string, int>> flagInfo;
    int flagKills[256] = {0};
    std::string loadPath(bz_ApiString path);
    std::string controlledFlagsFile; //path to file of controlled flags
};

BZ_PLUGIN(flagControl)

const char* flagControl::Name()
{
    return "flagControl v1.3.0";
}

void flagControl::Init(const char* config)
{
    Register(bz_ePlayerDieEvent);
    Register(bz_eFlagGrabbedEvent);
    Register(bz_eFlagDroppedEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);
    Register(bz_eBZDBChange);

    bz_registerCustomSlashCommand("flagcontrol", this);

    LoadControlledFlags(config);
    ReloadControlledFlagsFile();
    UpdateState();

    bz_registerCustomBZDBInt("_allFlagsAllowedAt", 4, 0, false);
    bz_registerCustomBZDBInt("_oneKillOnlyAt", 0, 0, false);

    allowFC = true;
    currentlyFC = false;
    playerCount = 0;
}

void flagControl::Cleanup()
{
    Flush();

    bz_removeCustomSlashCommand("flagcontrol");
}

void flagControl::Event(bz_EventData* eventData)
{
    switch (eventData->eventType)
    {
    case bz_ePlayerDieEvent:
    {
        bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*)eventData;

        if (allowFC && currentlyFC && data->killerID != data->playerID)
        {
            for (unsigned int i = 0; i < flagInfo.size(); i++)
            {
                if (data->flagKilledWith == flagInfo.at(i).first)
                {
                    flagKills[data->killerID] +=1;
                    int killsLeft = flagInfo.at(i).second - flagKills[data->killerID];
                    if(killsLeft <= 3 && killsLeft > 1)
                    {
                        bz_sendTextMessagef(BZ_SERVER, data->killerID, "%i kills left", killsLeft);
                    }
                    else if(killsLeft == 1)
                    {
                        bz_sendTextMessagef(BZ_SERVER, data->killerID, "%i kill left", killsLeft);
                    }

                    if(flagKills[data->killerID] >= flagInfo.at(i).second)
                    {
                        bz_removePlayerFlag(data->killerID);
                        bz_sendTextMessage(BZ_SERVER, data->killerID, "Flag control in effect; you've been forced to drop that flag. Find another!");
                    }
                    break;
                }
            }
            if (allowFC && playerCount <= bz_getBZDBInt("_oneKillOnlyAt"))
            {
                bz_removePlayerFlag(data->killerID);
                bz_sendTextMessage(BZ_SERVER, data->killerID, "Flag control in effect; you've been forced to drop that flag. Find another!");
            }
        }
    }
    break;

    case bz_eFlagGrabbedEvent:
    {
        // This event is called each time a flag is grabbed by a player
        bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*)eventData;

        if (allowFC && currentlyFC)
        {
            for (unsigned int i = 0; i < flagInfo.size(); i++)
            {
                if (data->flagType == flagInfo.at(i).first)
                {
                    if (flagInfo.at(i).second == 0)
                    {
                        bz_removePlayerFlag(data->playerID);
                        bz_sendTextMessage(BZ_SERVER, data->playerID, "You've been forced to drop that flag. Find another!");
                    }
                    else if(flagInfo.at(i).second == 1)
                    {
                        bz_sendTextMessagef(BZ_SERVER, data->playerID, "You get %i kill before your flag will be dropped", flagInfo.at(i).second);
                    }
                    else
                    {
                        bz_sendTextMessagef(BZ_SERVER, data->playerID, "You get %i kills before your flag will be dropped", flagInfo.at(i).second);
                    }
                }
            }
        }

        // Data
        // ----
        // (int)         playerID  - The player that grabbed the flag
        // (int)         flagID    - The flag ID that was grabbed
        // (const char*) flagType  - The flag abbreviation of the flag that was grabbed
        // (float[3])    pos       - The position at which the flag was grabbed
        // (double)      eventTime - This value is the local server time of the event.
    }
    break;

    case bz_eFlagDroppedEvent:
    {
        // This event is called each time a flag is dropped by a player.
        bz_FlagDroppedEventData_V1* data = (bz_FlagDroppedEventData_V1*)eventData;
        flagKills[data->playerID] = 0;
        // Data
        // ----
        // (int)          playerID  - The player that dropped the flag
        // (int)          flagID    - The flag ID that was dropped
        // (bz_ApiString) flagType  - The flag abbreviation of the flag that was grabbed
        // (float[3])     pos       - The position at which the flag was dropped
        // (double)       eventTime - This value is the local server time of the event.
    }
    break;

    case bz_ePlayerJoinEvent:
    {
        bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*)eventData;
        if (data->record->team != eObservers)
        {
            playerCount += 1;
            UpdateState();
        }
        
        if (currentlyFC && allowFC && bz_getPlayerCount() > 2)
            bz_sendTextMessage(BZ_SERVER, data->playerID, "Flag control in effect with fewer players online. Some flags might limit the number of kills.");
    }
    break;

    case bz_ePlayerPartEvent:
    {
        bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*)eventData;
        if (data->record->team != eObservers)
        {
            playerCount -= 1;
            UpdateState();
        }
    }
    break;

    case bz_eBZDBChange:
    {
        // This event is called each time a BZDB variable is changed
        bz_BZDBChangeData_V1* data = (bz_BZDBChangeData_V1*)eventData;
        if (data->key == "_allFlagsAllowedAt")
        {
            UpdateState();
        }
    }
    break;

    default:
        break;
    }

}


bool flagControl::SlashCommand(int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *params)
{
    if (command == "flagcontrol")
    {
        std::string name = bz_getPlayerCallsign(playerID);
        if (message == "list")
        {
            bz_sendTextMessagef(BZ_SERVER, playerID, "Flags controlled are: ");
            bz_sendTextMessagef(BZ_SERVER, playerID, "Flag  #Kills");
            bz_sendTextMessagef(BZ_SERVER, playerID, "------------");
            for (auto i : flagInfo)
            {
                bz_sendTextMessagef(BZ_SERVER, playerID, "%s %i", i.first.c_str(), i.second);
            }
        }
        else if (bz_hasPerm(playerID, "BAN") || bz_hasPerm(playerID, "FLAGCONTROL"))
        {
            if (message == "on")
            {
                if (allowFC)
                    bz_sendTextMessage(BZ_SERVER, playerID, "Flag Control is already set to \"on\".");
                else
                {
                    bz_sendTextMessage(BZ_SERVER, eAdministrators, ("Flag Conrol setting has been changed to \"on\" by " + name + ".").c_str());
                    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Flag Control has been enabled.");
                    allowFC = true;
                }
            }
            else if (message == "off")
            {
                if (!allowFC)
                    bz_sendTextMessage(BZ_SERVER, playerID, "Flag Control is already set to \"off\".");
                else
                {
                    bz_sendTextMessage(BZ_SERVER, eAdministrators, ("Flag Control setting has been changed to \"off\" by " + name + ".").c_str());
                    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Flag Control has been disabled.");
                    allowFC = false;
                }
            }
            else if (params->get(0) == "add")
            {
                std::string flag = params->get(1);
                for (auto& x : flag)
                {
                    x = toupper(x);
                }
                std::pair<std::string, int> flagDiv = SplitFlag(flag);
                auto removal = std::remove_if(flagInfo.begin(), flagInfo.end(), [flagDiv](std::pair<std::string, int> str){return str.first == flagDiv.first;});
                flagInfo.erase(removal, flagInfo.end());
                flagInfo.push_back(flagDiv);
                bz_sendTextMessage(BZ_SERVER, eAdministrators, (flag + " has been added as a controlled flag by " + name + ".").c_str());
            }
            else if (params->get(0) == "remove")
            {
                std::string flag = params->get(1);
                for (auto& x : flag)
                {
                    x = toupper(x);
                }
                std::pair<std::string, int> flagDiv = SplitFlag(flag);
                bool foundFlag = false;
                for (unsigned int i = 0; i < flagInfo.size(); i++)
                {
                    if(flagInfo.at(i).first == flagDiv.first)
                    {
                        foundFlag = true;
                    }
                }
                auto removal = std::remove_if(flagInfo.begin(), flagInfo.end(), [flagDiv](std::pair<std::string, int> str){return str.first == flagDiv.first;});
                flagInfo.erase(removal, flagInfo.end());
                if (foundFlag == true)
                {
                    bz_sendTextMessage(BZ_SERVER, eAdministrators, (flagDiv.first + " was removed by " + name).c_str());
                }
                else
                {
                    bz_sendTextMessage(BZ_SERVER, playerID, ("\"" + flag + "\" is not a controlled flag.").c_str());
                    bz_sendTextMessage(BZ_SERVER, playerID, ("You can add it by using \"add\" " + flag).c_str());
                }
            }
            else
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "Incorrect syntax! /flagcontrol on|off|list|add|remove.");
                bz_sendTextMessage(BZ_SERVER, playerID, "To change the number of players when in effect use /set _allFlagsAllowedAt #");
            }
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, (name + ", incorrect usage, /flagcontrol list").c_str());
        }
        return true;
    }
    return false;
}

void flagControl::UpdateState()
{
    if (playerCount >= bz_getBZDBInt("_allFlagsAllowedAt")) {
        if (allowFC && currentlyFC) {
            // Was on, now off - broadcast message
            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Enough players now online, flag control is no longer in effect");
            currentlyFC = false;
        }
    }
    else {
        if (allowFC && !currentlyFC && playerCount > 1) {
            // Was off, now on - broadcast message
            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Flag control in effect with fewer players online. Some flags might limit the number of kills.");
            currentlyFC = true;
        }
    }
}

void flagControl::LoadControlledFlags(const char *commandline)
{
    controlledFlagsFile = "";
    bz_APIStringList cmdLine;
    cmdLine.tokenize(commandline, "");
    if (cmdLine.size() >= 1)
    {
        controlledFlagsFile = loadPath(cmdLine.get(0));
    }
    
}

std::string flagControl::loadPath(bz_ApiString path)
{
    if (!bz_tolower(path.c_str()))
        return "";
    return path;
}

void flagControl::ReloadControlledFlagsFile()
{
    std::vector<std::string> controlledFlags;
    if (!controlledFlagsFile.empty())
    {
        controlledFlags = getFileTextLines(controlledFlagsFile);
        flagInfo.clear();
        for(unsigned int i = 0; i < controlledFlags.size(); i++)
        {
            flagInfo.push_back(SplitFlag(controlledFlags.at(i)));
        }
    }
}

std::pair<std::string, int> flagControl::SplitFlag(std::string rawFlag)
{
    char division[BUFFLEN];
    std::strncpy(division, rawFlag.c_str(), BUFFLEN);
    division[BUFFLEN-1]='\0';
    char * divided;
    std::pair<std::string, int> pair;
    pair.first = "";
    pair.second = 0;
    divided = std::strtok(division, " ,");
    if (divided != NULL)
    {
        pair.first = divided;
        divided = std::strtok(NULL, " ,");
        if (divided == NULL)
        {
            pair.second = 1;
            return pair;
        }
        pair.second = atoi(divided);
        return pair;
    }
    return pair;
}
// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
