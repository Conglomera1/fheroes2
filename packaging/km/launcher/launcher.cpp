// SPDX-License-Identifier: GPL-2.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include "settings.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    constexpr int rowHeight = 34;
    constexpr int firstReset = 100;
    constexpr int firstInput = 200;
    constexpr int languageId = 300;
    constexpr int englishId = 311;
    constexpr int frenchId = 312;
    constexpr int guideId = 301;
    constexpr int folderId = 302;
    constexpr int saveId = 303;
    constexpr int playId = 304;
    constexpr int quitId = 305;
    constexpr int defaultsId = 306;
    constexpr int saveProfileId = 307;
    constexpr int loadProfileId = 308;
    constexpr int sampleId = 309;
    constexpr int tabsId = 310;

    struct ComRelease
    {
        void operator()( IUnknown * object ) const { object->Release(); }
    };

    struct Row
    {
        HWND label{};
        HWND input{};
        HWND reset{};
        HWND spin{};
        std::wstring help;
    };

    std::wstring windowText( HWND window )
    {
        const int length = GetWindowTextLengthW( window );
        std::wstring text( static_cast<size_t>( length ) + 1, L'\0' );
        GetWindowTextW( window, text.data(), length + 1 );
        text.resize( static_cast<size_t>( length ) );
        return text;
    }

    std::wstring env( const wchar_t * name )
    {
        const DWORD length = GetEnvironmentVariableW( name, nullptr, 0 );
        if ( !length )
            return {};
        std::wstring value( length, L'\0' );
        const DWORD used = GetEnvironmentVariableW( name, value.data(), length );
        if ( used >= length )
            return {};
        value.resize( used );
        return value;
    }

    fs::path configDirectory()
    {
        const std::wstring home = env( L"HOME" );
        if ( !home.empty() )
            return fs::path( home ) / L".fheroes2";
        const std::wstring appData = env( L"APPDATA" );
        if ( !appData.empty() )
            return fs::path( appData ) / L"fheroes2";
        wchar_t folder[MAX_PATH]{};
        if ( SUCCEEDED( SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, folder ) ) )
            return fs::path( folder ) / L"fheroes2";
        throw std::runtime_error( "AppData" );
    }

    fs::path executableDirectory()
    {
        std::wstring path( 32768, L'\0' );
        const DWORD length = GetModuleFileNameW( nullptr, path.data(), static_cast<DWORD>( path.size() ) );
        if ( !length || length >= path.size() )
            throw std::runtime_error( "Launcher path" );
        path.resize( length );
        return fs::path( path ).parent_path();
    }

    std::string readFile( const fs::path & path )
    {
        std::ifstream file( path, std::ios::binary );
        if ( !file )
            throw std::runtime_error( "Read file" );
        std::string contents( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
        if ( file.bad() )
            throw std::runtime_error( "Read contents" );
        return contents;
    }

    void writeFile( const fs::path & path, const std::string & contents )
    {
        wchar_t temporary[MAX_PATH]{};
        if ( !GetTempFileNameW( path.parent_path().c_str(), L"km", 0, temporary ) )
            throw std::runtime_error( "Temporary file" );
        try {
            std::ofstream file( fs::path( temporary ), std::ios::binary | std::ios::trunc );
            file.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
            file.close();
            if ( !file || !MoveFileExW( temporary, path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                throw std::runtime_error( "Write settings" );
        }
        catch ( ... ) {
            DeleteFileW( temporary );
            throw;
        }
    }

    bool startGame( const fs::path & app, const fs::path & folder )
    {
        STARTUPINFOW startup{};
        startup.cb = sizeof( startup );
        PROCESS_INFORMATION process{};
        std::wstring command = L"\"" + app.wstring() + L"\"";
        if ( !CreateProcessW( app.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, folder.c_str(), &startup, &process ) )
            return false;
        CloseHandle( process.hThread );
        CloseHandle( process.hProcess );
        return true;
    }

    std::string narrowNumber( const std::wstring & text )
    {
        std::string result;
        for ( const wchar_t c : text ) {
            if ( c > 127 )
                return {};
            result += static_cast<char>( c );
        }
        return std::string( km::trim( result ) );
    }

    bool matches( const fs::path & first, const fs::path & second )
    {
        std::error_code error;
        return ( fs::equivalent( first, second, error ) && !error )
               || _wcsicmp( first.lexically_normal().c_str(), second.lexically_normal().c_str() ) == 0;
    }

    class Launcher
    {
    public:
        Launcher( HINSTANCE instance, const int show )
            : _instance( instance )
            , _installation( executableDirectory() )
            , _config( configDirectory() )
            , _ini( _config / L"knight_mod.ini" )
            , _localeFile( _config / L"knight_mod_launcher.language" )
        {
            _french = PRIMARYLANGID( GetUserDefaultUILanguage() ) == LANG_FRENCH;
            if ( fs::exists( _localeFile ) )
                _french = km::trim( readFile( _localeFile ) ) == "fr";
            else if ( fs::exists( _installation / L"KnightModLauncher.language" ) )
                _french = km::trim( readFile( _installation / L"KnightModLauncher.language" ) ) == "fr";

            const std::wstring args = GetCommandLineW();
            if ( args.find( L"--language=" ) != args.npos )
                _french = args.find( L"--language=fr" ) != args.npos;
            _values = km::defaults();
            if ( fs::exists( _ini ) ) {
                const auto content = readFile( _ini );
                km::load( content, _values, false );
                if ( _values[km::index( "mode" )] == 1 && content.find( "conscription.gold_per_peasant" ) == content.npos )
                    _values[km::index( "base" )] = 100;
            }
            _saved = _values;

            WNDCLASSW cls{};
            cls.hInstance = _instance;
            cls.lpfnWndProc = windowProc;
            cls.hCursor = LoadCursorW( nullptr, IDC_ARROW );
            cls.hIcon = LoadIconW( _instance, MAKEINTRESOURCEW( 1 ) );
            cls.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_BTNFACE + 1 );
            cls.lpszClassName = L"KnightModLauncher";
            if ( !RegisterClassW( &cls ) )
                throw std::runtime_error( "Window class" );
            cls.lpfnWndProc = panelProc;
            cls.lpszClassName = L"KnightModSettingsPanel";
            if ( !RegisterClassW( &cls ) )
                throw std::runtime_error( "Panel class" );

            RECT area{};
            SystemParametersInfoW( SPI_GETWORKAREA, 0, &area, 0 );
            const auto width = std::min( 840L, area.right - area.left );
            const auto height = std::min( 680L, area.bottom - area.top );
            _window = CreateWindowExW( WS_EX_CONTROLPARENT, L"KnightModLauncher", L"Knight Mod Launcher", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                       area.left + ( area.right - area.left - width ) / 2, area.top + ( area.bottom - area.top - height ) / 2,
                                       width, height, nullptr, nullptr, _instance, this );
            if ( !_window )
                throw std::runtime_error( "Window creation" );
            ShowWindow( _window, show );
            UpdateWindow( _window );
        }

        int run()
        {
            MSG message{};
            while ( GetMessageW( &message, nullptr, 0, 0 ) > 0 ) {
                if ( !IsDialogMessageW( _window, &message ) ) {
                    TranslateMessage( &message );
                    DispatchMessageW( &message );
                }
            }
            return static_cast<int>( message.wParam );
        }

    private:
        const wchar_t * tr( const wchar_t * fr, const wchar_t * en ) const { return _french ? fr : en; }

        static LRESULT CALLBACK windowProc( HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam )
        {
            auto * instance = reinterpret_cast<Launcher *>( GetWindowLongPtrW( hwnd, GWLP_USERDATA ) );
            if ( message == WM_NCCREATE ) {
                instance = static_cast<Launcher *>( reinterpret_cast<CREATESTRUCTW *>( lParam )->lpCreateParams );
                instance->_window = hwnd;
                SetWindowLongPtrW( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( instance ) );
            }
            if ( !instance )
                return DefWindowProcW( hwnd, message, wParam, lParam );
            try {
                return instance->process( message, wParam, lParam );
            }
            catch ( const std::exception & ) {
                instance->error( L"L’opération a échoué.", L"The operation failed." );
                return message == WM_CREATE ? -1 : 0;
            }
        }

        static LRESULT CALLBACK panelProc( HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam )
        {
            if ( message == WM_COMMAND || message == WM_NOTIFY || message == WM_VSCROLL || message == WM_MOUSEWHEEL )
                return SendMessageW( GetParent( hwnd ), message, wParam, lParam );
            return DefWindowProcW( hwnd, message, wParam, lParam );
        }

        void error( const wchar_t * fr, const wchar_t * en )
        {
            MessageBoxW( _window, tr( fr, en ), L"Knight Mod Launcher", MB_OK | MB_ICONERROR );
        }

        HWND control( const wchar_t * kind, const wchar_t * title, const DWORD flags, const int id = 0, HWND parent = nullptr,
                      const DWORD extended = 0 )
        {
            const HWND item = CreateWindowExW( extended, kind, title, WS_CHILD | WS_VISIBLE | flags, 0, 0, 0, 0, parent ? parent : _window,
                                               reinterpret_cast<HMENU>( static_cast<INT_PTR>( id ) ), _instance, nullptr );
            if ( !item )
                throw std::runtime_error( "Control creation" );
            SendMessageW( item, WM_SETFONT, reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ), TRUE );
            return item;
        }

        HWND button( const int id, HWND parent = nullptr )
        {
            return control( L"BUTTON", L"", WS_TABSTOP, id, parent );
        }

        HWND spin( HWND field, const int minimum, const int maximum )
        {
            const auto spinner = control( UPDOWN_CLASSW, L"", UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS, 0, _panel );
            SendMessageW( spinner, UDM_SETBUDDY, reinterpret_cast<WPARAM>( field ), 0 );
            SendMessageW( spinner, UDM_SETRANGE32, minimum, maximum );
            return spinner;
        }

        void create()
        {
            _updating = true;
            _language = control( L"BUTTON", L"change language", WS_TABSTOP, languageId );
            _tabs = control( WC_TABCONTROLW, L"", WS_TABSTOP, tabsId );
            for ( int i = 0; i < 3; ++i ) {
                TCITEMW item{};
                item.mask = TCIF_TEXT;
                item.pszText = const_cast<wchar_t *>( L" " );
                SendMessageW( _tabs, TCM_INSERTITEMW, i, reinterpret_cast<LPARAM>( &item ) );
            }
            _panel = control( L"KnightModSettingsPanel", L"", WS_CLIPCHILDREN | WS_VSCROLL, 0, _window, WS_EX_CONTROLPARENT );
            _tooltip = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _window, nullptr, _instance, nullptr );
            SendMessageW( _tooltip, TTM_SETMAXTIPWIDTH, 0, 430 );
            auto tool = tooltipInfo();
            SendMessageW( _tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>( &tool ) );
            SetTimer( _window, 1, 50, nullptr );

            for ( size_t i = 0; i < km::count; ++i ) {
                auto & row = _rows[i];
                const auto & setting = km::settings[i];
                const auto id = firstInput + static_cast<int>( i );
                row.label = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
                row.reset = button( firstReset + static_cast<int>( i ), _panel );
                switch ( setting.kind ) {
                case km::Kind::Toggle:
                    row.input = control( L"BUTTON", L"", BS_AUTOCHECKBOX | WS_TABSTOP, id, _panel );
                    break;
                case km::Kind::Choice:
                    row.input = control( L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, id, _panel );
                    break;
                default:
                    row.input = control( L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT, id, _panel, WS_EX_CLIENTEDGE );
                    if ( setting.kind == km::Kind::Integer )
                        row.spin = spin( row.input, static_cast<int>( setting.minimum ), static_cast<int>( setting.maximum ) );
                    break;
                }

            }
            for ( auto & title : _headings )
                title = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            _sampleLabel = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            _sample = control( L"EDIT", L"3", WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT, sampleId, _panel, WS_EX_CLIENTEDGE );
            _sampleSpin = spin( _sample, 0, static_cast<int>( Conscription::maxDailyOrder ) );
            _quote = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            _saveProfile = button( saveProfileId, _panel );
            _loadProfile = button( loadProfileId, _panel );
            _status = control( L"STATIC", L"", SS_CENTERIMAGE );
            _defaults = button( defaultsId );
            _guide = button( guideId );
            _folder = button( folderId );
            _save = button( saveId );
            _play = button( playId );
            _quit = button( quitId );
            _updating = false;
            localize();
            setControls();
        }

        std::wstring number( const double value, const bool decimal = false ) const
        {
            const auto canonical = km::format( value );
            std::wstring result( canonical.begin(), canonical.end() );
            if ( decimal && result.find( L'.' ) == result.npos )
                result += L".0";
            if ( _french )
                std::replace( result.begin(), result.end(), L'.', L',' );
            return result;
        }

        const wchar_t * choices( const km::Setting & setting ) const
        {
            return tr( setting.frenchChoices, setting.englishChoices );
        }

        void localize()
        {
            _updating = true;
            const wchar_t * names[]{ tr( L"Réglages", L"Settings" ), tr( L"Avancé", L"Advanced" ), tr( L"Divers", L"Miscellaneous" ) };
            for ( int i = 0; i < 3; ++i ) {
                TCITEMW item{};
                item.mask = TCIF_TEXT;
                item.pszText = const_cast<wchar_t *>( names[i] );
                SendMessageW( _tabs, TCM_SETITEMW, i, reinterpret_cast<LPARAM>( &item ) );
            }
            for ( size_t i = 0; i < km::count; ++i ) {
                const auto & setting = km::settings[i];
                const auto & row = _rows[i];
                SetWindowTextW( row.label, tr( setting.french, setting.english ) );
                SetWindowTextW( row.reset, tr( L"Défaut", L"Default" ) );
                if ( setting.kind == km::Kind::Choice ) {
                    SendMessageW( row.input, CB_RESETCONTENT, 0, 0 );
                    std::wstring labels( choices( setting ) );
                    size_t start = 0;
                    for ( size_t end = labels.find( L'|' ); ; end = labels.find( L'|', start ) ) {
                        const std::wstring label = labels.substr( start, end - start );
                        SendMessageW( row.input, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( label.c_str() ) );
                        if ( end == labels.npos )
                            break;
                        start = end + 1;
                    }
                }
            }
            SetWindowTextW( _headings[0], tr( L"Taxation", L"Taxation" ) );
            SetWindowTextW( _headings[1], tr( L"Conscription", L"Conscription" ) );
            SetWindowTextW( _headings[2], tr( L"Formation", L"Training" ) );
            SetWindowTextW( _headings[3], tr( L"Passage aux terrains d’entraînement", L"Convert to Training Grounds" ) );
            SetWindowTextW( _headings[4], tr( L"Retour à la Ferme", L"Restore the Farm" ) );
            SetWindowTextW( _headings[5], tr( L"Prérequis des terrains d’entraînement", L"Training Grounds requirements" ) );
            SetWindowTextW( _headings[6], tr( L"Affichage", L"Display" ) );
            SetWindowTextW( _headings[7], tr( L"Profils", L"Profiles" ) );
            SetWindowTextW( _sampleLabel, tr( L"Paysans de garnison (exemple)", L"Garrison Peasants (example)" ) );
            SetWindowTextW( _saveProfile, tr( L"Enregistrer un profil", L"Save Profile" ) );
            SetWindowTextW( _loadProfile, tr( L"Charger un profil", L"Load Profile" ) );
            SetWindowTextW( _defaults, tr( L"Tout remettre par défaut", L"Restore all defaults" ) );
            SetWindowTextW( _guide, tr( L"Guide d’équilibrage", L"Balance guide" ) );
            SetWindowTextW( _folder, tr( L"Ouvrir le dossier de configuration", L"Open configuration folder" ) );
            SetWindowTextW( _save, tr( L"Enregistrer", L"Save" ) );
            SetWindowTextW( _play, tr( L"Jouer", L"Play" ) );
            SetWindowTextW( _quit, tr( L"Quitter", L"Quit" ) );
            _updating = false;
        }

        void setField( const size_t i )
        {
            const auto & setting = km::settings[i];
            const auto & row = _rows[i];
            if ( setting.kind == km::Kind::Toggle )
                SendMessageW( row.input, BM_SETCHECK, _values[i] != 0 ? BST_CHECKED : BST_UNCHECKED, 0 );
            else if ( setting.kind == km::Kind::Choice )
                SendMessageW( row.input, CB_SETCURSEL, static_cast<WPARAM>( _values[i] ), 0 );
            else
                SetWindowTextW( row.input, number( _values[i], setting.kind == km::Kind::Decimal ).c_str() );
        }

        void setControls()
        {
            _updating = true;
            for ( size_t i = 0; i < km::count; ++i )
                setField( i );
            _updating = false;
            layout();
            refresh();
        }

        std::optional<double> fieldValue( const size_t i ) const
        {
            const auto & row = _rows[i];
            switch ( km::settings[i].kind ) {
            case km::Kind::Toggle:
                return SendMessageW( row.input, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1.0 : 0.0;
            case km::Kind::Choice:
                return static_cast<double>( SendMessageW( row.input, CB_GETCURSEL, 0, 0 ) );
            default:
                return km::parse( narrowNumber( windowText( row.input ) ), km::settings[i] );
            }
        }

        bool capture()
        {
            for ( size_t i = 0; i < km::count; ++i ) {
                const auto value = fieldValue( i );
                if ( !value ) {
                    const auto & rule = km::settings[i];
                    const std::wstring range = std::wstring( tr( rule.french, rule.english ) ) + L"  (" + number( rule.minimum ) + L" – "
                                               + number( rule.maximum ) + L")";
                    MessageBoxW( _window, range.c_str(), tr( L"Valeur invalide", L"Invalid value" ), MB_OK | MB_ICONWARNING );
                    SetFocus( _rows[i].input );
                    return false;
                }
                _values[i] = *value;
            }
            return true;
        }

        static void place( HWND item, const int x, const int y, const int width, const int height )
        {
            MoveWindow( item, x, y, width, height, TRUE );
            ShowWindow( item, SW_SHOWNA );
        }

        void layout()
        {
            if ( !_panel )
                return;
            RECT rect{};
            GetClientRect( _window, &rect );
            const int w = rect.right;
            const int h = rect.bottom;
            place( _language, w - 159, 10, 149, 25 );
            place( _tabs, 10, 41, w - 20, 29 );
            const int panelHeight = std::max( 1, h - 169 );
            place( _panel, 10, 70, w - 20, panelHeight );
            RECT panelRect{};
            GetClientRect( _panel, &panelRect );
            const int panelWidth = panelRect.right;
            const int resetX = std::max( 30, panelWidth - 217 );
            const int inputX = resetX + 75;
            const int choiceWidth = std::max( 80, panelWidth - inputX - 12 );
            const int tab = static_cast<int>( SendMessageW( _tabs, TCM_GETCURSEL, 0, 0 ) );
            for ( const auto & row : _rows ) {
                for ( const HWND item : { row.label, row.input, row.reset, row.spin } )
                    if ( item )
                        ShowWindow( item, SW_HIDE );
            }
            for ( const HWND item : _headings )
                ShowWindow( item, SW_HIDE );
            for ( const HWND item : { _sampleLabel, _sample, _sampleSpin, _quote, _saveProfile, _loadProfile } )
                ShowWindow( item, SW_HIDE );

            int y = 8 - _scroll;
            const auto heading = [&]( const size_t i ) {
                place( _headings[i], 10, y, panelWidth - 20, 27 );
                y += 28;
            };
            const auto row = [&]( const size_t i ) {
                const auto & entry = _rows[i];
                const auto kind = km::settings[i].kind;
                place( entry.label, 15, y, resetX - 22, 27 );
                place( entry.reset, resetX, y, 68, 26 );
                const int inputWidth = kind == km::Kind::Toggle ? 24 : choiceWidth;
                const int inputHeight = kind == km::Kind::Choice ? 160 : 25;
                place( entry.input, inputX, y, inputWidth - ( entry.spin ? 18 : 0 ), inputHeight );
                if ( entry.spin )
                    place( entry.spin, inputX + inputWidth - 18, y, 18, 25 );
                y += rowHeight;
            };
            const auto named = [&]( const char * id ) { row( km::index( id ) ); };
            if ( tab <= 0 ) {
                heading( 0 );
                named( "tax" );
                named( "cap" );
                heading( 1 );
                named( "mode" );
                const auto mode = _values[km::index( "mode" )];
                if ( mode != 0 )
                    named( "base" );
                if ( mode == 1 ) {
                    named( "odd" );
                    named( "even" );
                }
                if ( mode == 2 ) {
                    for ( const auto id : { "increment", "step", "multiplier", "uniform" } )
                        named( id );
                    place( _sampleLabel, 15, y, resetX - 22, 25 );
                    place( _sample, resetX, y, 80, 25 );
                    place( _sampleSpin, resetX + 80, y, 18, 25 );
                    place( _quote, resetX + 104, y, panelWidth - resetX - 106, 25 );
                    y += rowHeight;
                }
            }
            else if ( tab == 1 ) {
                heading( 2 );
                for ( const auto id : { "success", "archers", "pikemen", "swordsmen" } )
                    named( id );
                heading( 3 );
                for ( const auto id : { "tg0", "tg1", "tg2" } )
                    named( id );
                heading( 4 );
                for ( const auto id : { "farm0", "farm1", "farm2" } )
                    named( id );
                heading( 5 );
                for ( size_t i = 0; i < km::count; ++i ) {
                    if ( std::string_view( km::settings[i].key ).starts_with( "training_grounds.require_" ) )
                        row( i );
                }
            }
            else {
                heading( 6 );
                named( "artwork" );
                heading( 7 );
                place( _saveProfile, 15, y, 175, 27 );
                place( _loadProfile, 199, y, 175, 27 );
                y += rowHeight;
            }
            const int content = y + _scroll + 8;
            _scrollMaximum = std::max( 0, content - panelHeight );
            if ( _scroll > _scrollMaximum ) {
                _scroll = _scrollMaximum;
                layout();
                return;
            }
            SCROLLINFO scroll{ sizeof( SCROLLINFO ), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL, 0, content - 1,
                               static_cast<UINT>( panelHeight ), _scroll, 0 };
            SetScrollInfo( _panel, SB_VERT, &scroll, TRUE );

            place( _status, 12, h - 94, w - 24, 21 );
            place( _defaults, 10, h - 67, 177, 25 );
            place( _guide, 193, h - 67, 140, 25 );
            place( _folder, 339, h - 67, 226, 25 );
            place( _save, w - 273, h - 35, 87, 26 );
            place( _play, w - 180, h - 35, 79, 26 );
            place( _quit, w - 95, h - 35, 85, 26 );
        }

        void status( const wchar_t * text )
        {
            SetWindowTextW( _status, text );
        }

        void refresh()
        {
            const auto text = narrowNumber( windowText( _sample ) );
            uint32_t count = 0;
            const auto [end, ec] = std::from_chars( text.data(), text.data() + text.size(), count );
            const bool valid = ec == std::errc{} && end == text.data() + text.size() && count <= Conscription::maxDailyOrder;
            const bool validPricing = fieldValue( km::index( "base" ) ) && fieldValue( km::index( "increment" ) ) && fieldValue( km::index( "step" ) )
                                      && fieldValue( km::index( "multiplier" ) );
            const auto quote = valid && validPricing ? std::to_wstring( km::quote( _values, count ) ) + tr( L" or", L" gold" ) : L"—";
            SetWindowTextW( _quote, quote.c_str() );
            status( _values == _saved ? L"" : tr( L"Modifications non enregistrées", L"Unsaved changes" ) );
        }

        TOOLINFOW tooltipInfo() const
        {
            TOOLINFOW tool{};
            tool.cbSize = sizeof( tool );
            tool.hwnd = _window;
            tool.uId = 1;
            tool.uFlags = TTF_TRACK;
            tool.lpszText = const_cast<wchar_t *>( L"" );
            return tool;
        }

        void updateTooltip()
        {
            POINT cursor{};
            size_t rowIndex = km::count;
            if ( GetForegroundWindow() == _window && GetCursorPos( &cursor ) ) {
                const HWND hovered = WindowFromPoint( cursor );
                if ( hovered ) {
                    for ( size_t i = 0; i < km::count; ++i ) {
                        const auto & row = _rows[i];
                        if ( hovered == row.label || hovered == row.input || hovered == row.reset || hovered == row.spin ) {
                            rowIndex = i;
                            break;
                        }
                    }
                }
            }

            auto tool = tooltipInfo();
            const ULONGLONG now = GetTickCount64();
            if ( rowIndex != _hoveredRow ) {
                if ( _showingTooltip )
                    SendMessageW( _tooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>( &tool ) );
                _showingTooltip = false;
                _hoveredRow = rowIndex;
                _hoverStarted = now;
            }
            if ( rowIndex == km::count || _showingTooltip || now - _hoverStarted < 550 )
                return;

            auto & row = _rows[rowIndex];
            const auto & setting = km::settings[rowIndex];
            const double initial = rowIndex == km::index( "base" ) && _values[km::index( "mode" )] == 1 ? 100 : setting.initial;
            row.help = std::wstring( tr( setting.frenchHelp, setting.englishHelp ) ) + tr( L"\r\nPar défaut : ", L"\r\nDefault: " )
                       + number( initial, setting.kind == km::Kind::Decimal );
            tool.lpszText = row.help.data();
            SendMessageW( _tooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>( &tool ) );
            SendMessageW( _tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM( cursor.x, cursor.y + 20 ) );
            SendMessageW( _tooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>( &tool ) );
            _showingTooltip = true;
        }

        std::optional<fs::path> profilePath( const bool save )
        {
            fs::create_directories( _config );
            IFileDialog * nativeDialog = nullptr;
            if ( FAILED( CoCreateInstance( save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARGS( &nativeDialog ) ) ) )
                throw std::runtime_error( "File dialog" );
            const std::unique_ptr<IFileDialog, ComRelease> dialog( nativeDialog );
            IShellItem * nativeFolder = nullptr;
            if ( FAILED( SHCreateItemFromParsingName( _config.c_str(), nullptr, IID_PPV_ARGS( &nativeFolder ) ) ) )
                throw std::runtime_error( "Profile folder" );
            const std::unique_ptr<IShellItem, ComRelease> folder( nativeFolder );
            const COMDLG_FILTERSPEC filter{ tr( L"Profil Knight Mod (*.ini)", L"Knight Mod Profile (*.ini)" ), L"*.ini" };
            dialog->SetTitle( save ? tr( L"Enregistrer sous", L"Save As" ) : tr( L"Charger un profil", L"Load Profile" ) );
            dialog->SetFileTypes( 1, &filter );
            dialog->SetDefaultExtension( L"ini" );
            if ( FAILED( dialog->SetFolder( folder.get() ) ) )
                throw std::runtime_error( "Initial folder" );
            FILEOPENDIALOGOPTIONS options{};
            dialog->GetOptions( &options );
            dialog->SetOptions( options | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR | FOS_PATHMUSTEXIST
                                | ( save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST ) );
            if ( save )
                dialog->SetFileName( L"knight-mod-profile.ini" );
            const HRESULT result = dialog->Show( _window );
            if ( result == HRESULT_FROM_WIN32( ERROR_CANCELLED ) )
                return {};
            IShellItem * nativeFile = nullptr;
            if ( FAILED( result ) || FAILED( dialog->GetResult( &nativeFile ) ) )
                throw std::runtime_error( "Profile path" );
            const std::unique_ptr<IShellItem, ComRelease> file( nativeFile );
            wchar_t * nativePath = nullptr;
            if ( FAILED( file->GetDisplayName( SIGDN_FILESYSPATH, &nativePath ) ) )
                throw std::runtime_error( "Profile filename" );
            const std::unique_ptr<wchar_t, decltype( &CoTaskMemFree )> path( nativePath, CoTaskMemFree );
            return fs::path( path.get() );
        }

        void save( const bool play )
        {
            if ( !capture() )
                return;
            fs::create_directories( _config );
            const std::string existing = fs::exists( _ini ) ? readFile( _ini ) : "# Knight Mod settings\r\n";
            writeFile( _ini, km::updateIni( existing, _values ) );
            _saved = _values;
            status( tr( L"Réglages enregistrés.", L"Settings saved." ) );
            if ( play && !startGame( _installation / L"fheroes2KM.exe", _installation ) )
                error( L"Impossible de lancer fheroes2KM.exe. Place le lanceur à côté du jeu.", L"Could not start fheroes2KM.exe. Place the launcher next to the game." );
        }

        void profile( const bool saveProfile )
        {
            if ( !capture() )
                return;
            const auto path = profilePath( saveProfile );
            if ( !path )
                return;
            if ( saveProfile ) {
                if ( matches( *path, _ini ) ) {
                    error( L"Choisis un nom différent de knight_mod.ini.", L"Choose a name other than knight_mod.ini." );
                    return;
                }
                writeFile( *path, km::updateIni( "# Knight Mod profile\r\n", _values ) );
                status( tr( L"Profil enregistré.", L"Profile saved." ) );
            }
            else {
                if ( !km::load( readFile( *path ), _values, true ) ) {
                    error( L"Ce profil ne contient pas de réglages Knight Mod valides.", L"This profile does not contain valid Knight Mod settings." );
                    return;
                }
                setControls();
                status( tr( L"Profil chargé. Enregistre les réglages ou clique sur Jouer pour les appliquer.",
                            L"Profile loaded. Save the settings or click Play to apply them." ) );
            }
        }

        void open( const fs::path & path )
        {
            const auto result = reinterpret_cast<INT_PTR>( ShellExecuteW( _window, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL ) );
            if ( result <= 32 )
                error( L"Impossible d’ouvrir le fichier ou le dossier.", L"Could not open the file or folder." );
        }

        void changeLanguage()
        {
            const HMENU languages = CreatePopupMenu();
            if ( !languages )
                return;
            AppendMenuW( languages, MF_STRING | ( _french ? MF_UNCHECKED : MF_CHECKED ), englishId, L"English" );
            AppendMenuW( languages, MF_STRING | ( _french ? MF_CHECKED : MF_UNCHECKED ), frenchId, L"Français" );
            RECT bounds{};
            GetWindowRect( _language, &bounds );
            const BOOL chosen = TrackPopupMenu( languages, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN, bounds.right, bounds.bottom, 0, _window, nullptr );
            DestroyMenu( languages );
            if ( !chosen || !capture() )
                return;
            _french = chosen == frenchId;
            localize();
            setControls();
            fs::create_directories( _config );
            writeFile( _localeFile, _french ? "fr\n" : "en\n" );
        }

        void command( const int id, const int notification )
        {
            if ( _updating )
                return;
            if ( id >= firstReset && id < firstReset + static_cast<int>( km::count ) && notification == BN_CLICKED ) {
                const size_t i = static_cast<size_t>( id - firstReset );
                _values[i] = i == km::index( "base" ) && _values[km::index( "mode" )] == 1 ? 100 : km::settings[i].initial;
                _updating = true;
                setField( i );
                _updating = false;
                if ( i == km::index( "mode" ) ) {
                    _scroll = 0;
                    layout();
                }
                refresh();
                return;
            }
            if ( id >= firstInput && id < firstInput + static_cast<int>( km::count ) ) {
                if ( notification != EN_CHANGE && notification != BN_CLICKED && notification != CBN_SELCHANGE )
                    return;
                const size_t i = static_cast<size_t>( id - firstInput );
                if ( const auto value = fieldValue( i ) )
                    _values[i] = *value;
                if ( i == km::index( "mode" ) ) {
                    _scroll = 0;
                    layout();
                }
                refresh();
                return;
            }
            switch ( id ) {
            case languageId:
                if ( notification == BN_CLICKED )
                    changeLanguage();
                break;
            case sampleId:
                if ( notification == EN_CHANGE )
                    refresh();
                break;
            case defaultsId:
                _values = km::defaults();
                setControls();
                break;
            case saveId:
            case playId:
                save( id == playId );
                break;
            case saveProfileId:
            case loadProfileId:
                profile( id == saveProfileId );
                break;
            case guideId:
                open( _installation / tr( L"Guide_de_Test_et_Equilibrage.pdf", L"Testing_and_Balance_Guide.pdf" ) );
                break;
            case folderId:
                fs::create_directories( _config );
                open( _config );
                break;
            case quitId:
                SendMessageW( _window, WM_CLOSE, 0, 0 );
                break;
            }
        }

        LRESULT process( const UINT message, const WPARAM wParam, const LPARAM lParam )
        {
            switch ( message ) {
            case WM_CREATE:
                create();
                return 0;
            case WM_SIZE:
                if ( wParam != SIZE_MINIMIZED )
                    layout();
                return 0;
            case WM_GETMINMAXINFO: {
                auto * info = reinterpret_cast<MINMAXINFO *>( lParam );
                info->ptMinTrackSize = { 600, 390 };
                return 0;
            }
            case WM_COMMAND:
                command( LOWORD( wParam ), HIWORD( wParam ) );
                return 0;
            case WM_NOTIFY: {
                const auto & info = *reinterpret_cast<NMHDR *>( lParam );
                if ( info.hwndFrom == _tabs && info.code == TCN_SELCHANGE ) {
                    _scroll = 0;
                    layout();
                }
                return 0;
            }
            case WM_TIMER:
                updateTooltip();
                return 0;
            case WM_MOUSEWHEEL:
                _scroll = std::clamp( _scroll - GET_WHEEL_DELTA_WPARAM( wParam ) * rowHeight * 3 / WHEEL_DELTA, 0, _scrollMaximum );
                layout();
                return 0;
            case WM_VSCROLL:
                if ( lParam )
                    return 0;
                switch ( LOWORD( wParam ) ) {
                case SB_LINEUP: _scroll -= rowHeight; break;
                case SB_LINEDOWN: _scroll += rowHeight; break;
                case SB_PAGEUP: _scroll -= rowHeight * 8; break;
                case SB_PAGEDOWN: _scroll += rowHeight * 8; break;
                case SB_THUMBPOSITION:
                case SB_THUMBTRACK: _scroll = HIWORD( wParam ); break;
                }
                _scroll = std::clamp( _scroll, 0, _scrollMaximum );
                layout();
                return 0;
            case WM_CLOSE:
                for ( size_t i = 0; i < km::count; ++i ) {
                    const auto value = fieldValue( i );
                    if ( !value || *value != _saved[i] ) {
                        if ( IDYES != MessageBoxW( _window, tr( L"Quitter sans enregistrer les modifications ?", L"Quit without saving your changes?" ),
                                                   L"Knight Mod Launcher", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION ) )
                            return 0;
                        break;
                    }
                }
                DestroyWindow( _window );
                return 0;
            case WM_DESTROY:
                PostQuitMessage( 0 );
                return 0;
            }
            return DefWindowProcW( _window, message, wParam, lParam );
        }

        HINSTANCE _instance;
        fs::path _installation;
        fs::path _config;
        fs::path _ini;
        fs::path _localeFile;
        std::array<Row, km::count> _rows{};
        std::array<HWND, 8> _headings{};
        km::Values _values{};
        km::Values _saved{};
        HWND _window{}, _panel{}, _tabs{}, _tooltip{}, _language{}, _sample{}, _sampleLabel{}, _sampleSpin{}, _quote{};
        HWND _defaults{}, _save{}, _play{}, _quit{}, _guide{}, _folder{}, _saveProfile{}, _loadProfile{}, _status{};
        int _scroll{}, _scrollMaximum{};
        size_t _hoveredRow = km::count;
        ULONGLONG _hoverStarted{};
        bool _french{}, _updating{}, _showingTooltip{};
    };
}

int WINAPI wWinMain( HINSTANCE instance, HINSTANCE, PWSTR, int show )
{
    const INITCOMMONCONTROLSEX controls{ sizeof( INITCOMMONCONTROLSEX ), ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_WIN95_CLASSES };
    InitCommonControlsEx( &controls );
    if ( FAILED( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) ) )
        return 1;
    int result = 1;
    try {
        Launcher launcher( instance, show );
        result = launcher.run();
    }
    catch ( const std::exception & ) {
        MessageBoxW( nullptr, L"Could not start Knight Mod Launcher. / Impossible d’ouvrir le lanceur.", L"Knight Mod Launcher", MB_ICONERROR );
    }
    CoUninitialize();
    return result;
}
