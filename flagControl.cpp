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

#include "bzfsAPI.h"
#include "plugin_files.h"

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

    bool allowFC;
    bool currentlyFC;

    std::vector<std::string> controlledFlags;
    std::string loadPath(bz_ApiString path);
    std::string controlledFlagsFile; //path to file of controlled flags
};

BZ_PLUGIN(flagControl)

const char* flagControl::Name()
{
    return "flagControl";
}

void flagControl::Init(const char* config)
{
    Register(bz_ePlayerDieEvent);
    Register(bz_eFlagGrabbedEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);

    bz_registerCustomSlashCommand("flagcontrol", this);

    LoadControlledFlags(config);
    ReloadControlledFlagsFile();
    UpdateState();

    bz_registerCustomBZDBInt("_allFlagsAllowedAt", 4, 0, false);

    allowFC = true;
    currentlyFC = false;
    if (controlledFlags.empty())
    {
        bz_debugMessage(2, "Warning! :: Flag control :: Failed to load flags");
    }
    else
    {
        bz_debugMessagef(2, "DEBUG :: flagControl :: Loaded %d flags", controlledFlags.size());
    }
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
        // This event is called each time a flag is grabbed by a player
        bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*)eventData;

        if (allowFC && currentlyFC)
        {
            for (unsigned int i = 0; i < controlledFlags.size(); i++)
            {
                if (data->flagKilledWith == controlledFlags.at(i))
                {
                    bz_removePlayerFlag(data->killerID);
                    bz_sendTextMessage(BZ_SERVER, data->killerID, "Flag control in effect; you've been forced to drop that flag. Find another!");
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

    case bz_ePlayerJoinEvent:
    {
        // This event is called each time a player joins the game
        bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*)eventData;
        UpdateState();
        if (currentlyFC && allowFC && bz_getPlayerCount() != 1)
            bz_sendTextMessage(BZ_SERVER, data->playerID, "Flag control in effect with fewer players online. Some flags can only be used for a single kill.");
    }
    break;

    case bz_ePlayerPartEvent:
    {
        UpdateState();
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
        if (!bz_hasPerm(playerID, "FLAGCONTROL"))
            bz_sendTextMessage(BZ_SERVER, playerID, (name + ", you do not have permission to use the /flagcontrol command.").c_str());
        else
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
            else if (message == "list")
            {
                bz_sendTextMessagef(BZ_SERVER, playerID, "Flags controlled are: ");
                for (auto i : controlledFlags)
                {
                    bz_sendTextMessage(BZ_SERVER, playerID, i.c_str());
                }
            }
            else if (params->get(0) == "add")
            {
                std::string flag = params->get(1);
                bool foundFlag = false;
                bool isFlag = false;

                for (unsigned int i = 0; i < bz_getNumFlags(); i++)
                {
                    if (flag == bz_getFlagName(i).c_str())
                        isFlag = true;
                }
                for (unsigned int i = 0; i < controlledFlags.size(); i++)
                {
                    if (flag == controlledFlags.at(i))
                        foundFlag = true;
                }

                if (isFlag && !foundFlag)
                {
                    controlledFlags.push_back(flag);
                    bz_sendTextMessage(BZ_SERVER, playerID, (flag + " has been added as a controlled flag.").c_str());
                }
                else if (foundFlag && isFlag)
                    bz_sendTextMessage(BZ_SERVER, playerID, (flag + " is already a controlled flag.").c_str());
                else if (!isFlag)
                    bz_sendTextMessage(BZ_SERVER, playerID, (flag + " is not a flag, try again.").c_str());
            }
            else if (params->get(0) == "remove")
            {
                std::string flag = params->get(1);
                bool foundFlag = false;
                for (unsigned int i = 0; i < controlledFlags.size(); i++)
                {
                    if (controlledFlags.at(i) == flag)
                    {
                        controlledFlags.erase(controlledFlags.begin() + i);
                        bz_sendTextMessage(BZ_SERVER, playerID, (flag + " has been removed from being a controlled flag.").c_str());
                        foundFlag = true;
                    }
                }
                if (!foundFlag)
                {
                    bz_sendTextMessage(BZ_SERVER, playerID, ("\"" + flag + "\" is not a controlled flag.").c_str());
                    bz_sendTextMessage(BZ_SERVER, playerID, ("You can add it by using \"add\" " + flag).c_str());
                }
            }
            else
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "Incorrect syntax! /flagcontrol on|off|list|add|remove.");
            }
        }
        return true;
    }
    return false;
}

void flagControl::UpdateState()
{

    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);
    int ji = 0;
    for (unsigned int i = 0; i < playerList->size(); i++) {

        if (bz_getPlayerByIndex(playerList->get(i))->team != eObservers)
        {
            ji++;
        }
    }

    bz_deleteIntList(playerList);
    if (ji >= bz_getBZDBInt("_allFlagsAllowedAt")) {
        if (allowFC && currentlyFC == true) {
            // Was on, now off - broadcast message
            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Enough players now online, flag control is no longer in effect");
        }
        currentlyFC = false;
    }
    else {
        if (allowFC && currentlyFC == false) {
            // Was off, now on - broadcast message
            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Flag control in effect with fewer players online. Some flags can only be used for a single kill.");
        }
        currentlyFC = true;
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
    if (bz_tolower(path.c_str()) == "null")
        return "";
    return path;
}

void flagControl::ReloadControlledFlagsFile()
{
    controlledFlags.clear();
    if (!controlledFlagsFile.empty())
    {
        controlledFlags = getFileTextLines(controlledFlagsFile);
    }
}
// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
