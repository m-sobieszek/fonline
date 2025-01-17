//      __________        ___               ______            _
//     / ____/ __ \____  / (_)___  ___     / ____/___  ____ _(_)___  ___
//    / /_  / / / / __ \/ / / __ \/ _ \   / __/ / __ \/ __ `/ / __ \/ _ `
//   / __/ / /_/ / / / / / / / / /  __/  / /___/ / / / /_/ / / / / /  __/
//  /_/    \____/_/ /_/_/_/_/ /_/\___/  /_____/_/ /_/\__, /_/_/ /_/\___/
//                                                  /____/
// FOnline Engine
// https://fonline.ru
// https://github.com/cvet/fonline
//
// MIT License
//
// Copyright (c) 2006 - 2022, Anton Tsvetinskiy aka cvet <cvet@tut.by>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "ClientConnection.h"

#include "Log.h"
#include "MsgFiles.h"
#include "Networking.h"

ClientConnection::ClientConnection(NetConnection* net_connection) : Bin {net_connection->Bin}, BinLocker {net_connection->BinLocker}, Bout {net_connection->Bout}, BoutLocker {net_connection->BoutLocker}, _netConnection {net_connection}
{
    _netConnection->AddRef();
}

ClientConnection::~ClientConnection()
{
    _netConnection->Disconnect();
    _netConnection->Release();
}

auto ClientConnection::GetIp() const -> uint
{
    return _netConnection->GetIp();
}

auto ClientConnection::GetHost() const -> string_view
{
    return _netConnection->GetHost();
}

auto ClientConnection::GetPort() const -> ushort
{
    return _netConnection->GetPort();
}

auto ClientConnection::IsHardDisconnected() const -> bool
{
    return _netConnection->IsDisconnected();
}

auto ClientConnection::IsGracefulDisconnected() const -> bool
{
    return _gracefulDisconnected;
}

void ClientConnection::DisableCompression()
{
    NON_CONST_METHOD_HINT();

    _netConnection->Dispatch();
}

void ClientConnection::Dispatch()
{
    NON_CONST_METHOD_HINT();

    _netConnection->Dispatch();
}

void ClientConnection::HardDisconnect()
{
    NON_CONST_METHOD_HINT();

    _netConnection->Disconnect();
}

void ClientConnection::GracefulDisconnect()
{
    _gracefulDisconnected = true;

    CONNECTION_OUTPUT_BEGIN(this);
    Bout << NETMSG_DISCONNECT;
    CONNECTION_OUTPUT_END(this);
}

void ClientConnection::Send_CustomMessage(uint msg)
{
    CONNECTION_OUTPUT_BEGIN(this);
    Bout << msg;
    CONNECTION_OUTPUT_END(this);
}

void ClientConnection::Send_TextMsg(uint num_str)
{
    CONNECTION_OUTPUT_BEGIN(this);
    Bout << NETMSG_MSG;
    Bout << static_cast<uint>(0);
    Bout << static_cast<uchar>(SAY_NETMSG);
    Bout << static_cast<ushort>(TEXTMSG_GAME);
    Bout << num_str;
    CONNECTION_OUTPUT_END(this);
}

void ClientConnection::Send_TextMsgLex(uint num_str, string_view lexems)
{
    const uint msg_len = NETMSG_MSG_SIZE + sizeof(msg_len) + NetBuffer::STRING_LEN_SIZE + static_cast<uint>(lexems.length());

    CONNECTION_OUTPUT_BEGIN(this);
    Bout << NETMSG_MSG_LEX;
    Bout << msg_len;
    Bout << static_cast<uint>(0);
    Bout << static_cast<uchar>(SAY_NETMSG);
    Bout << static_cast<ushort>(TEXTMSG_GAME);
    Bout << num_str;
    Bout << lexems;
    CONNECTION_OUTPUT_END(this);
}
