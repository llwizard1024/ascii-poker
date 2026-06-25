#include "i18n.h"

#include <array>
#include <string>

namespace poker::client {

namespace {

Language g_language = Language::English;

constexpr std::array<std::array<const char*, 2>, static_cast<size_t>(Msg::Count)> kTable = { {
    { "ASCII Poker", "ASCII Покер" },
    { "Enter your name", "Введите имя" },
    { "Name:", "Имя:" },
    { "(type below)", "(введите ниже)" },
    { "Press Enter or [Join] when ready.", "Нажмите Enter или [Войти], когда готовы." },
    { "Waiting for server connection...", "Ожидание подключения к серверу..." },
    { "Connection lost", "Соединение потеряно" },
    { "The server closed the connection or the network failed.", "Сервер закрыл соединение или сеть недоступна." },
    { "Press [Reconnect] to try again.", "Нажмите [Переподключить] для повторной попытки." },
    { "Lobby", "Лобби" },
    { "No rooms yet. Refresh or create one.", "Комнат пока нет. Обновите список или создайте." },
    { "Create room", "Создать комнату" },
    { "Max players:", "Макс. игроков:" },
    { "Press Enter on [Create] to confirm", "Нажмите Enter на [Создать] для подтверждения" },
    { "Blinds: {0}/{1}  |  Starting stack: {2}", "Блайнды: {0}/{1}  |  Стартовый стек: {2}" },
    { "Waiting for players", "Ожидание игроков" },
    { "Players at the table:", "Игроки за столом:" },
    { " [host]", " [хост]" },
    { "Need at least 2 players to start.", "Нужно минимум 2 игрока для старта." },
    { "Press [Start] when ready.", "Нажмите [Старт], когда готовы." },
    { "Waiting for {0} to start.", "Ожидание старта от {0}." },
    { "{0}/{1} players", "{0}/{1} игроков" },
    { "Game also auto-starts when the room is full.", "Игра также стартует автоматически при заполнении комнаты." },
    { "Waiting for game state...", "Ожидание состояния игры..." },
    { "Phase: {0}", "Фаза: {0}" },
    { "Pot:", "Банк:" },
    { "Table bet:", "Ставка стола:" },
    { "Community cards:", "Общие карты:" },
    { "Your hand:", "Ваша рука:" },
    { "Hand:", "Комбинация:" },
    { "Table:", "Стол:" },
    { "(no players)", "(нет игроков)" },
    { "Event log", "Журнал событий" },
    { "Controls:", "Управление:" },
    { "Lobby: j/k or Up/Down  |  Game: f/c/r  |  Log: [ ]  |  Language: L", "Лобби: j/k  |  Игра: f/c/r  |  Лог: [ ]  |  Язык: L (не в поле ввода)" },
    { "Language: {0}  [L] toggle", "Язык: {0}  [L] переключить" },
    { "YOUR TURN  |  Stack: {0}", "ВАШ ХОД  |  Стек: {0}" },
    { "Winner:", "Победитель:" },
    { "Pot:", "Банк:" },
    { " Join ", " Войти " },
    { " Quit ", " Выход " },
    { " Refresh ", " Обновить " },
    { " Create ", " Создать " },
    { " Leave ", " Покинуть " },
    { " Start ", " Старт " },
    { " Reconnect ", " Переподключить " },
    { " Confirm ", " Подтвердить " },
    { " Cancel ", " Отмена " },
    { " Fold ", " Фолд " },
    { " Check ", " Чек " },
    { " Call ", " Колл " },
    { " Raise ", " Рейз " },
    { " Min ", " Мин " },
    { " 1/2 ", " 1/2 " },
    { " Pot ", " Банк " },
    { " All-in ", " Олл-ин " },
    { " Lang ", " Язык " },
    { " ▲ ", " ▲ " },
    { " ▼ ", " ▼ " },
    { "Connecting...", "Подключение..." },
    { "Connected", "Подключено" },
    { "Connection failed", "Ошибка подключения" },
    { "Disconnected", "Отключено" },
    { "Connected to {0}", "Подключено к {0}" },
    { "Connecting to {0}...", "Подключение к {0}..." },
    { "Could not connect to {0}", "Не удалось подключиться к {0}" },
    { "Connection lost", "Соединение потеряно" },
    { "Welcome, {0}", "Добро пожаловать, {0}" },
    { "Room list updated ({0} rooms)", "Список комнат обновлён ({0} комн.)" },
    { "Table roster updated", "Состав стола обновлён" },
    { "Joined room {0} ({1})", "Вход в комнату {0} ({1})" },
    { "You are the host — start when ready", "Вы хост — начните, когда готовы" },
    { "Your turn!", "Ваш ход!" },
    { "Hand won by {0} ({1} chips)", "Раздачу выиграл {0} ({1} фишек)" },
    { "Left room {0}", "Покинули комнату {0}" },
    { "In lobby", "В лобби" },
    { "{0} to act", "Ход: {0}" },
    { "Action ignored: not your turn", "Действие отклонено: не ваш ход" },
    { "Action not available: {0}", "Действие недоступно: {0}" },
    { "Not connected to server", "Нет подключения к серверу" },
    { "Enter a player name", "Введите имя игрока" },
    { "Name must be 1-32 characters", "Имя: от 1 до 32 символов" },
    { "Joining as {0}...", "Вход как {0}..." },
    { "Authenticating...", "Аутентификация..." },
    { "Invalid max players value", "Некорректное число игроков" },
    { "Room name required, max players 2-255", "Нужно имя комнаты, игроков 2–255" },
    { "Invalid raise amount", "Некорректная сумма рейза" },
    { "Raise must be between {0} and {1}", "Рейз от {0} до {1}" },
    { "Cannot join — game already in progress", "Нельзя войти — игра уже идёт" },
    { "Select a room to join", "Выберите комнату" },
    { "Pre-Flop", "Префлоп" },
    { "Flop", "Флоп" },
    { "Turn", "Тёрн" },
    { "River", "Ривер" },
    { "Showdown", "Шоудаун" },
    { "High card", "Старшая карта" },
    { "Pair", "Пара" },
    { "Two pair", "Две пары" },
    { "Three of a kind", "Тройка" },
    { "Straight", "Стрит" },
    { "Flush", "Флеш" },
    { "Full house", "Фулл-хаус" },
    { "Four of a kind", "Каре" },
    { "Straight flush", "Стрит-флеш" },
    { " [fold]", " [фолд]" },
    { " [all-in]", " [олл-ин]" },
    { " (you)", " (вы)" },
    { " chips", " фишек" },
    { " bet:", " ставка:" },
    { "Waiting for board...", "Ожидание борда..." },
    { "Room {0}: {1}  [{2}/{3}]", "Комната {0}: {1}  [{2}/{3}]" },
    { "  (in game)", "  (в игре)" },
    { "  |  To call: {0}", "  |  Колл: {0}" },
    { "  |  Raise {0}-{1}", "  |  Рейз {0}-{1}" },
    { "English", "English" },
    { "Русский", "Русский" },
    { " [D]", " [Д]" },
    { " [SB]", " [МБ]" },
    { " [BB]", " [ББ]" },
    { "Host:", "Хост:" },
    { "Port:", "Порт:" },
    { "Next hand starting...", "Следующая раздача..." },
    { "Pocket {0}s", "Карманные {0}" },
    { "Suited {0}", "Одномастные {0}" },
    { "Broadway cards", "Бродвейные карты" },
    { "Connected cards", "Связанные карты" },
    { "Ace high", "Старший туз" },
    { "High card hand", "Старшая карта" },
    { "Room {0}", "Комната {0}" },
    { "Error: {0}", "Ошибка: {0}" },
    { " Call {0} ", " Колл {0} " },
} };

static_assert(kTable.size() == static_cast<size_t>(Msg::Count));

std::string replace_all(std::string text, std::string_view placeholder, std::string_view value)
{
    const auto pos = text.find(placeholder);
    if (pos == std::string::npos) {
        return text;
    }
    text.replace(pos, placeholder.size(), value);
    return text;
}

std::string apply_args(std::string text, std::string_view arg0, std::string_view arg1, std::string_view arg2, std::string_view arg3)
{
    if (!arg0.empty()) {
        text = replace_all(std::move(text), "{0}", arg0);
    }
    if (!arg1.empty()) {
        text = replace_all(std::move(text), "{1}", arg1);
    }
    if (!arg2.empty()) {
        text = replace_all(std::move(text), "{2}", arg2);
    }
    if (!arg3.empty()) {
        text = replace_all(std::move(text), "{3}", arg3);
    }
    return text;
}

} // namespace

Language current_language()
{
    return g_language;
}

void set_language(const Language language)
{
    g_language = language;
}

Language parse_language(const std::string_view code)
{
    if (code == "ru" || code == "RU" || code == "russian") {
        return Language::Russian;
    }
    return Language::English;
}

std::string language_code(const Language language)
{
    return language == Language::Russian ? "ru" : "en";
}

std::string language_display_name(const Language language)
{
    return tr(language == Language::Russian ? Msg::LangRussian : Msg::LangEnglish);
}

std::string tr(const Msg id)
{
    const auto index = static_cast<size_t>(id);
    if (index >= kTable.size()) {
        return "?";
    }
    const auto lang = static_cast<size_t>(g_language);
    return kTable[index][lang];
}

std::string tr(const Msg id, const std::string_view arg0)
{
    return apply_args(tr(id), arg0, {}, {}, {});
}

std::string tr(const Msg id, const std::string_view arg0, const std::string_view arg1)
{
    return apply_args(tr(id), arg0, arg1, {}, {});
}

std::string tr(const Msg id, const std::string_view arg0, const std::string_view arg1, const std::string_view arg2)
{
    return apply_args(tr(id), arg0, arg1, arg2, {});
}

std::string tr(const Msg id, const std::string_view arg0, const std::string_view arg1, const std::string_view arg2, const std::string_view arg3)
{
    return apply_args(tr(id), arg0, arg1, arg2, arg3);
}

std::string tr_phase(const poker::protocol::GamePhase phase)
{
    switch (phase) {
    case poker::protocol::GamePhase::PreFlop:
        return tr(Msg::PhasePreFlop);
    case poker::protocol::GamePhase::Flop:
        return tr(Msg::PhaseFlop);
    case poker::protocol::GamePhase::Turn:
        return tr(Msg::PhaseTurn);
    case poker::protocol::GamePhase::River:
        return tr(Msg::PhaseRiver);
    case poker::protocol::GamePhase::Showdown:
        return tr(Msg::PhaseShowdown);
    }
    return "?";
}

std::string tr_action(const poker::protocol::Action action)
{
    switch (action) {
    case poker::protocol::Action::Fold:
        return tr(Msg::BtnFold);
    case poker::protocol::Action::Check:
        return tr(Msg::BtnCheck);
    case poker::protocol::Action::Call:
        return tr(Msg::BtnCall);
    case poker::protocol::Action::Raise:
        return tr(Msg::BtnRaise);
    }
    return "?";
}

std::string tr_hand_category(const poker::HandCategory category)
{
    switch (category) {
    case poker::HandCategory::HighCard:
        return tr(Msg::HandHighCard);
    case poker::HandCategory::OnePair:
        return tr(Msg::HandPair);
    case poker::HandCategory::TwoPair:
        return tr(Msg::HandTwoPair);
    case poker::HandCategory::ThreeOfKind:
        return tr(Msg::HandThreeOfKind);
    case poker::HandCategory::Straight:
        return tr(Msg::HandStraight);
    case poker::HandCategory::Flush:
        return tr(Msg::HandFlush);
    case poker::HandCategory::FullHouse:
        return tr(Msg::HandFullHouse);
    case poker::HandCategory::FourOfKind:
        return tr(Msg::HandFourOfKind);
    case poker::HandCategory::StraightFlush:
        return tr(Msg::HandStraightFlush);
    }
    return "?";
}

} // namespace poker::client
