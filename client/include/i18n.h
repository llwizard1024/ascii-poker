#pragma once

#include <poker/hand_evaluator.h>
#include <poker/protocol.h>

#include <string>
#include <string_view>

namespace poker::client {

enum class Language {
    English,
    Russian
};

enum class Msg : unsigned {
    AppTitle,
    EnterYourName,
    NameLabel,
    TypeBelow,
    PressJoinWhenReady,
    WaitingForConnection,
    ConnectionLostTitle,
    ConnectionLostDetail,
    PressReconnect,
    Lobby,
    NoRoomsYet,
    CreateRoom,
    MaxPlayersLabel,
    PressCreateConfirm,
    BlindsInfo,
    WaitingForPlayers,
    PlayersAtTable,
    HostTag,
    NeedTwoPlayers,
    PressStartWhenReady,
    WaitingForHostStart,
    PlayersCount,
    AutoStartWhenFull,
    WaitingForGameState,
    PhasePrefix,
    PotLabel,
    TableBetLabel,
    CommunityCards,
    YourHand,
    HandPrefix,
    TableLabel,
    NoPlayers,
    EventLog,
    Controls,
    HotkeyHint,
    LanguageHint,
    YourTurnBar,
    WinnerPrefix,
    PotAmountPrefix,
    BtnJoin,
    BtnQuit,
    BtnRefresh,
    BtnCreate,
    BtnLeave,
    BtnStart,
    BtnReconnect,
    BtnConfirm,
    BtnCancel,
    BtnFold,
    BtnCheck,
    BtnCall,
    BtnRaise,
    BtnMin,
    BtnHalfPot,
    BtnPot,
    BtnAllIn,
    BtnLang,
    BtnRoomUp,
    BtnRoomDown,
    ConnConnecting,
    ConnConnected,
    ConnFailed,
    ConnDisconnected,
    ConnectedTo,
    ConnectingTo,
    CouldNotConnect,
    ConnectionLostShort,
    WelcomeUser,
    RoomListUpdated,
    TableRosterUpdated,
    JoinedRoomLog,
    YouAreHost,
    YourTurn,
    HandWonBy,
    LeftRoomLog,
    InLobby,
    ToActSuffix,
    ActionIgnored,
    ActionNotAvailable,
    NotConnected,
    EnterPlayerName,
    NameLengthInvalid,
    JoiningAs,
    Authenticating,
    InvalidMaxPlayers,
    RoomNameRequired,
    InvalidRaiseAmount,
    RaiseBetween,
    CannotJoinInGame,
    SelectRoom,
    PhasePreFlop,
    PhaseFlop,
    PhaseTurn,
    PhaseRiver,
    PhaseShowdown,
    HandHighCard,
    HandPair,
    HandTwoPair,
    HandThreeOfKind,
    HandStraight,
    HandFlush,
    HandFullHouse,
    HandFourOfKind,
    HandStraightFlush,
    PlayerFolded,
    PlayerAllIn,
    PlayerYou,
    PlayerChips,
    PlayerBet,
    WaitingForBoard,
    RoomLabel,
    RoomInGame,
    ToCallSuffix,
    RaiseRange,
    LangEnglish,
    LangRussian,
    PlayerDealer,
    PlayerSmallBlind,
    PlayerBigBlind,
    HostLabel,
    PortLabel,
    NextHandSoon,
    PreflopPocketPair,
    PreflopSuited,
    PreflopBroadway,
    PreflopConnector,
    PreflopAce,
    PreflopHighCard,
    HeaderRoom,
    ErrorPrefix,
    BtnCallAmount,
    Count
};

Language current_language();
void set_language(Language language);
Language parse_language(std::string_view code);
std::string language_code(Language language);
std::string language_display_name(Language language);

std::string tr(Msg id);
std::string tr(Msg id, std::string_view arg0);
std::string tr(Msg id, std::string_view arg0, std::string_view arg1);
std::string tr(Msg id, std::string_view arg0, std::string_view arg1, std::string_view arg2);
std::string tr(Msg id, std::string_view arg0, std::string_view arg1, std::string_view arg2, std::string_view arg3);

std::string tr_phase(poker::protocol::GamePhase phase);
std::string tr_action(poker::protocol::Action action);
std::string tr_hand_category(poker::HandCategory category);

} // namespace poker::client
