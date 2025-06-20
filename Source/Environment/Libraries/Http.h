#pragma once

#include "../../Miscellaneous/Includes.h"
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setglobal(L, Name);

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

inline auto replace(std::string& str, const std::string& from, const std::string& to) -> bool
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

inline std::string replaceAll(std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

inline std::string downloadUrl(lua_State* L, std::string URL)
{
    HINTERNET interwebs = InternetOpen("Roblox/WinInet", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
    HINTERNET urlFile = nullptr;
    std::string rtn;
    lua_getglobal(L, "game");
    lua_getfield(L, -1, "JobId");
    std::string jobId;
    if (lua_type(L, -1) == LUA_TSTRING) {
        jobId = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
        return "Error: JobId is not a string!";
    }
    lua_getfield(L, -1, "GameId");
    std::string gameId;
    if (lua_type(L, -1) == LUA_TNUMBER) {
        gameId = std::to_string(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    else {
        lua_pop(L, 1);
        return "Error: GameId is not a number!";
    }
    lua_getfield(L, -1, "PlaceId");
    std::string placeId;
    if (lua_type(L, -1) == LUA_TNUMBER) {
        placeId = std::to_string(lua_tonumber(L, -1));
        lua_pop(L, 2);
    }
    else {
        lua_pop(L, 2);
        return "Error: PlaceId is not a number!";
    }
    const std::string headers = "Roblox-Session-Id: {\"GameId\":\"" + jobId + "\",\"PlaceId\":\"" + placeId + "\"}\r\nRoblox-Game-Id: \"" + gameId + "\"";
    if (interwebs)
    {
        urlFile = InternetOpenUrl(interwebs, URL.c_str(), headers.c_str(), static_cast<DWORD>(headers.length()), INTERNET_FLAG_RELOAD, 0);
        if (urlFile)
        {
            char buffer[2000]{};
            DWORD bytesRead = 0;
            do
            {
                if (!InternetReadFile(urlFile, buffer, sizeof(buffer), &bytesRead)) {
                    return "Error: Failed to read from the URL!";
                }
                if (bytesRead > 0) {
                    rtn.append(buffer, bytesRead);
                }
                memset(buffer, 0, sizeof(buffer));
            } while (bytesRead);
            InternetCloseHandle(urlFile);
        }
        else {
            return "Error: Failed to open URL!";
        }
        InternetCloseHandle(interwebs);
    }
    return replaceAll(rtn, "|n", "\r\n");
}

namespace Library {
    namespace Http {
        inline int httpget(lua_State* L) {
            luaL_checktype(L, 2, LUA_TSTRING);
            std::string url = lua_tostring(L, 2);
            std::string result = downloadUrl(L, url);
            lua_pushlstring(L, result.data(), result.size());
            return 1;
        }

        inline int request(lua_State* L) {
            luaL_checktype(L, 1, LUA_TTABLE);

            lua_getfield(L, 1, "Url");
            if (lua_type(L, -1) != LUA_TSTRING) {
                luaL_error(L, "Invalid or no 'Url' field specified in request table");
                return 0;
            }
            auto url = lua_tostring(L, -1);
            if (url == nullptr || strlen(url) == 0) {
                luaL_error(L, "'Url' field cannot be empty");
                return 0;
            }
            lua_pop(L, 1);

            lua_getfield(L, 1, "Method");
            auto method = std::string(luaL_optstring(L, -1, "POST"));
            lua_pop(L, 1);

            std::string body;
            lua_getfield(L, 1, "Body");
            if (lua_isstring(L, -1)) {
                body = luaL_checkstring(L, -1);
            }
            lua_pop(L, 1);

            lua_getfield(L, 1, "Headers");
            bool hasHeaders = lua_istable(L, -1);
            lua_getglobal(L, "game");
            lua_getfield(L, -1, "JobId");
            std::string jobId = lua_tostring(L, -1);
            if (jobId.empty()) {
                luaL_error(L, "Invalid 'JobId' in game context");
                return 0;
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "GameId");
            std::string gameId = lua_tostring(L, -1);
            if (gameId.empty()) {
                luaL_error(L, "Invalid 'GameId' in game context");
                return 0;
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "PlaceId");
            std::string placeId = lua_tostring(L, -1);
            if (placeId.empty()) {
                luaL_error(L, "Invalid 'PlaceId' in game context");
                return 0;
            }
            lua_pop(L, 2);

            std::string headers = "Roblox-Session-Id: {\"GameId\":\"" + jobId + "\",\"PlaceId\":\"" + placeId + "\"}\r\n";
            headers += "Roblox-Game-Id: \"" + gameId + "\"\r\n";
            if (hasHeaders) {
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    const char* headerKey = luaL_checkstring(L, -2);
                    const char* headerValue = luaL_checkstring(L, -1);
                    if (headerKey && headerValue) {
                        headers += headerKey;
                        headers += ": ";
                        headers += headerValue;
                        headers += "\r\n";
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);

            lua_getfield(L, 1, "Cookies");
            bool hasCookies = lua_istable(L, -1);
            std::string cookies;
            if (hasCookies) {
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    const char* cookieName = luaL_checkstring(L, -2);
                    const char* cookieValue = luaL_checkstring(L, -1);
                    if (cookieName && cookieValue) {
                        cookies += cookieName;
                        cookies += "=";
                        cookies += cookieValue;
                        cookies += "; ";
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);

            headers += "User-Agent: SkidBase/1.0.1\r\n";

            HW_PROFILE_INFO hwProfileInfo;
            if (!GetCurrentHwProfile(&hwProfileInfo)) {
                luaL_error(L, "Failed to retrieve hardware profile.");
                return 0;
            }

            std::string hwid = hwProfileInfo.szHwProfileGuid;
            if (hwid.empty()) {
                luaL_error(L, "Invalid HWID.");
                return 0;
            }

            replace(hwid, "}", "");
            replace(hwid, "{", "");
            headers += "Fingerprint: SkidBase-Fingerprint\r\n";
            if (!hwid.empty()) {
                headers += "Exploit-Guid: " + hwid + "\r\n";
                headers += "SkidBase-Fingerprint: " + hwid + "\r\n";
            }
            else {
                headers += "Exploit-Guid: Unknown\r\n";
            }

            HINTERNET hSession = InternetOpen("SkidBase", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            if (!hSession) {
                luaL_error(L, "Failed to open Internet session.");
                return 0;
            }

            HINTERNET hConnect = InternetOpenUrl(hSession, url, headers.c_str(), (DWORD)headers.length(), INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
            if (!hConnect) {
                InternetCloseHandle(hSession);
                luaL_error(L, "Failed to connect to URL.");
                return 0;
            }

            DWORD statusCode = 0;
            DWORD length = sizeof(DWORD);
            if (!HttpQueryInfo(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL)) {
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hSession);
                luaL_error(L, "Failed to retrieve status code.");
                return 0;
            }

            lua_newtable(L);

            std::string responseText;
            char buffer[4096] = {};
            DWORD bytesRead;

            while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0) {
                if (bytesRead > 0) {
                    responseText.append(buffer, bytesRead);
                }
                else {
                    break;
                }
            }

            lua_pushlstring(L, responseText.data(), responseText.length());
            lua_setfield(L, -2, "Body");

            lua_pushinteger(L, statusCode);
            lua_setfield(L, -2, "StatusCode");

            lua_pushboolean(L, statusCode >= 200 && statusCode < 300);
            lua_setfield(L, -2, "Success");

            lua_newtable(L);
            if (hasHeaders) {
                lua_pushlstring(L, "Headers", 7);
                lua_newtable(L);
                lua_pushlstring(L, headers.c_str(), headers.length());
                lua_setfield(L, -2, "AllHeaders");
                lua_settable(L, -3);
            }
            lua_setfield(L, -2, "Headers");

            InternetCloseHandle(hConnect);
            InternetCloseHandle(hSession);

            return 1;
        }

        inline void RegisterLibrary(lua_State* L) {
            lua_newtable(L);
            RegisterMember(L, httpget, "get");
            RegisterMember(L, request, "request");
            lua_setglobal(L, "http");

            RegisterFunction(L, httpget, "httpGet");
            RegisterFunction(L, request, "request");
            RegisterFunction(L, request, "http_request");
        }
    }
}