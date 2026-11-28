// SPDX-License-Identifier: GPL-2.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include "settings.h"
#include "settings_files.h"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    constexpr int rowHeight = 37;
    constexpr int headerHeight = 66;
    constexpr int tabsHeight = 41;
    constexpr int footerHeight = 102;
    constexpr int pageMargin = 22;
    constexpr int minWidth = 620;
    constexpr COLORREF ink = RGB( 23, 36, 54 );
    constexpr COLORREF navy = RGB( 23, 44, 71 );
    constexpr COLORREF blue = RGB( 36, 83, 142 );
    constexpr COLORREF muted = RGB( 77, 93, 113 );
    constexpr COLORREF white = RGB( 255, 255, 255 );
    constexpr COLORREF edge = RGB( 174, 190, 209 );
    constexpr COLORREF separator = RGB( 217, 225, 233 );
    constexpr COLORREF wash = RGB( 238, 244, 251 );
    constexpr COLORREF footer = RGB( 245, 248, 252 );
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
    constexpr int settingsTabId = 310;
    constexpr int advancedTabId = 313;
    constexpr int miscellaneousTabId = 314;
    constexpr int readmeId = 315;
    constexpr int conversionId = 316;
    constexpr int requirementsId = 317;
    constexpr int panelId = 318;

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

    void fill( HDC dc, const RECT & bounds, const COLORREF color )
    {
        SetDCBrushColor( dc, color );
        FillRect( dc, &bounds, static_cast<HBRUSH>( GetStockObject( DC_BRUSH ) ) );
    }

    void surface( HDC dc, const RECT & bounds, const COLORREF background, const COLORREF border, const int radius )
    {
        const int saved = SaveDC( dc );
        SelectObject( dc, GetStockObject( DC_BRUSH ) );
        SelectObject( dc, GetStockObject( DC_PEN ) );
        SetDCBrushColor( dc, background );
        SetDCPenColor( dc, border );
        RoundRect( dc, bounds.left, bounds.top, bounds.right, bounds.bottom, radius, radius );
        RestoreDC( dc, saved );
    }

    struct UiResources
    {
        HFONT body{}, compactFont{}, strong{}, title{};
        HICON icon{};
        HBRUSH paper{ CreateSolidBrush( white ) };
        HBRUSH callout{ CreateSolidBrush( wash ) };
        HBRUSH footerFill{ CreateSolidBrush( footer ) };

        ~UiResources()
        {
            for ( const auto font : { body, compactFont, strong, title } )
                if ( font )
                    DeleteObject( font );
            for ( const auto brush : { paper, callout, footerFill } )
                if ( brush )
                    DeleteObject( brush );
            if ( icon )
                DestroyIcon( icon );
        }
    };

    std::wstring windowText( HWND window )
    {
        const int length = GetWindowTextLengthW( window );
        std::wstring text( static_cast<size_t>( length ) + 1, L'\0' );
        GetWindowTextW( window, text.data(), length + 1 );
        text.resize( static_cast<size_t>( length ) );
        return text;
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

    class Launcher
    {
    public:
        Launcher( HINSTANCE instance, const int show )
            : _instance( instance )
            , _installation( km::files::installationDirectory() )
            , _modDirectory( _installation / L"Knight Mod" )
            , _config( km::files::configurationDirectory() )
            , _ini( _config / L"knight_mod.ini" )
            , _localeFile( _config / L"knight_mod_launcher.language" )
        {
            _french = PRIMARYLANGID( GetUserDefaultUILanguage() ) == LANG_FRENCH;
            if ( km::files::isFile( _localeFile ) )
                _french = km::trim( km::files::readText( _localeFile ) ) == "fr";
            else if ( km::files::isFile( _modDirectory / L"KnightModLauncher.language" ) )
                _french = km::trim( km::files::readText( _modDirectory / L"KnightModLauncher.language" ) ) == "fr";
            else if ( km::files::isFile( _installation / L"KnightModLauncher.language" ) )
                _french = km::trim( km::files::readText( _installation / L"KnightModLauncher.language" ) ) == "fr";

            const std::wstring args = GetCommandLineW();
            if ( args.find( L"--language=" ) != args.npos )
                _french = args.find( L"--language=fr" ) != args.npos;
            _values = km::defaults();
            if ( km::files::isFile( _ini ) )
                km::load( km::files::readText( _ini ), _values, false );
            _saved = _values;

            if ( const auto dc = GetDC( nullptr ) ) {
                _dpi = GetDeviceCaps( dc, LOGPIXELSX );
                ReleaseDC( nullptr, dc );
            }
            _ui.body = makeFont( 14, FW_NORMAL );
            _ui.compactFont = makeFont( 12, FW_NORMAL );
            _ui.strong = makeFont( 14, FW_BOLD );
            _ui.title = makeFont( 23, FW_BOLD );
            _ui.icon = static_cast<HICON>( LoadImageW( _instance, MAKEINTRESOURCEW( 1 ), IMAGE_ICON, px( 40 ), px( 40 ), 0 ) );

            WNDCLASSW cls{};
            cls.hInstance = _instance;
            cls.lpfnWndProc = windowProc;
            cls.hCursor = LoadCursorW( nullptr, IDC_ARROW );
            cls.hIcon = LoadIconW( _instance, MAKEINTRESOURCEW( 1 ) );
            cls.lpszClassName = L"KnightModLauncher";
            if ( !RegisterClassW( &cls ) )
                throw std::runtime_error( "Window class" );
            cls.lpfnWndProc = panelProc;
            cls.lpszClassName = L"KnightModSettingsPanel";
            if ( !RegisterClassW( &cls ) )
                throw std::runtime_error( "Panel class" );

            RECT area{};
            SystemParametersInfoW( SPI_GETWORKAREA, 0, &area, 0 );
            constexpr DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
            RECT preferred{ 0, 0, px( 648 ), px( 675 ) };
            AdjustWindowRectEx( &preferred, style, FALSE, WS_EX_CONTROLPARENT );
            const auto width = std::min( preferred.right - preferred.left, area.right - area.left );
            const auto height = std::min( preferred.bottom - preferred.top, area.bottom - area.top );
            _window = CreateWindowExW( WS_EX_CONTROLPARENT, L"KnightModLauncher", L"Knight Mod Launcher", style,
                                       area.left + ( area.right - area.left - width ) / 2, area.top + ( area.bottom - area.top - height ) / 2,
                                       width, height, nullptr, nullptr, _instance, this );
            if ( !_window )
                throw std::runtime_error( "Window creation" );
            ShowWindow( _window, show );
            fitToContent();
            UpdateWindow( _window );
        }

        int run()
        {
            MSG message{};
            while ( GetMessageW( &message, nullptr, 0, 0 ) > 0 ) {
                const bool tabKey = message.message == WM_KEYDOWN && message.wParam == VK_TAB;
                if ( !IsDialogMessageW( _window, &message ) ) {
                    TranslateMessage( &message );
                    DispatchMessageW( &message );
                }
                else if ( tabKey )
                    revealFocusedField();
            }
            return static_cast<int>( message.wParam );
        }

    private:
        const wchar_t * tr( const wchar_t * fr, const wchar_t * en ) const { return _french ? fr : en; }

        int px( const int value ) const { return MulDiv( value, _dpi, 96 ); }
        int dip( const LONG value ) const { return MulDiv( value, 96, _dpi ); }

        HFONT makeFont( const int height, const int weight ) const
        {
            return CreateFontW( -px( height ), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI" );
        }

        RECT bounds( const int x, const int y, const int width, const int height ) const
        {
            return { px( x ), px( y ), px( x + width ), px( y + height ) };
        }

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
            const auto parent = GetParent( hwnd );
            if ( message == WM_ERASEBKGND )
                return 1;
            if ( message == WM_PAINT ) {
                auto * launcher = reinterpret_cast<Launcher *>( GetWindowLongPtrW( parent, GWLP_USERDATA ) );
                if ( launcher ) {
                    launcher->paintPanel( hwnd );
                    return 0;
                }
            }
            if ( message == WM_COMMAND || message == WM_NOTIFY || message == WM_VSCROLL || message == WM_MOUSEWHEEL || message == WM_DRAWITEM
                 || message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT )
                return SendMessageW( parent, message, wParam, lParam );
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
            SendMessageW( item, WM_SETFONT, reinterpret_cast<WPARAM>( _ui.body ), TRUE );
            return item;
        }

        HWND button( const int id, HWND parent = nullptr )
        {
            return control( L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, id, parent );
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
            _language = button( languageId );
            SetWindowTextW( _language, L"Change language" );
            _tabs = { button( settingsTabId ), button( advancedTabId ), button( miscellaneousTabId ) };
            _panel = control( L"KnightModSettingsPanel", L"", WS_CLIPCHILDREN | WS_VSCROLL, panelId, _window, WS_EX_CONTROLPARENT );
            _tooltip = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _window, nullptr, _instance, nullptr );
            SendMessageW( _tooltip, TTM_SETMAXTIPWIDTH, 0, px( 400 ) );
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
                    row.input = control( L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP, id, _panel );
                    SendMessageW( row.input, CB_SETITEMHEIGHT, static_cast<WPARAM>( -1 ), px( 24 ) );
                    SendMessageW( row.input, CB_SETITEMHEIGHT, 0, px( 28 ) );
                    break;
                default:
                    row.input = control( L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT, id, _panel );
                    SendMessageW( row.input, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM( px( 5 ), px( 5 ) ) );
                    if ( setting.kind == km::Kind::Integer )
                        row.spin = spin( row.input, static_cast<int>( setting.minimum ), static_cast<int>( setting.maximum ) );
                    break;
                }
            }
            for ( auto & title : _headings ) {
                title = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
                SendMessageW( title, WM_SETFONT, reinterpret_cast<WPARAM>( _ui.strong ), TRUE );
            }
            _capHint = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            _incrementHint = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            for ( const auto hint : { _capHint, _incrementHint } )
                SendMessageW( hint, WM_SETFONT, reinterpret_cast<WPARAM>( _ui.compactFont ), TRUE );
            _conversion = button( conversionId, _panel );
            _requirements = button( requirementsId, _panel );
            _sampleLabel = control( L"STATIC", L"", SS_CENTERIMAGE, 0, _panel );
            _sample = control( L"EDIT", L"3", WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT, sampleId, _panel );
            SendMessageW( _sample, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM( px( 5 ), px( 5 ) ) );
            _sampleSpin = spin( _sample, 0, static_cast<int>( Conscription::maxDailyOrder ) );
            _quote = control( L"STATIC", L"", SS_CENTERIMAGE | SS_RIGHT, 0, _panel );
            SendMessageW( _quote, WM_SETFONT, reinterpret_cast<WPARAM>( _ui.strong ), TRUE );
            _saveProfile = button( saveProfileId, _panel );
            _loadProfile = button( loadProfileId, _panel );
            _status = control( L"STATIC", L"", SS_CENTERIMAGE | SS_ENDELLIPSIS );
            _defaults = button( defaultsId );
            _guide = button( guideId );
            _readme = button( readmeId );
            SetWindowTextW( _readme, L"Readme" );
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
            for ( size_t i = 0; i < _tabs.size(); ++i )
                SetWindowTextW( _tabs[i], names[i] );
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
            SetWindowTextW( _conversion, tr( L"Coûts de conversion", L"Conversion costs" ) );
            SetWindowTextW( _requirements, tr( L"Prérequis des terrains d’entraînement", L"Training Grounds requirements" ) );
            SetWindowTextW( _incrementHint, tr( L"1,0 = le coût de base", L"1.0 = the base cost" ) );
            SetWindowTextW( _sampleLabel, tr( L"Paysans de garnison (exemple)", L"Garrison Peasants (example)" ) );
            SetWindowTextW( _saveProfile, tr( L"Enregistrer un profil", L"Save Profile" ) );
            SetWindowTextW( _loadProfile, tr( L"Charger un profil", L"Load Profile" ) );
            SetWindowTextW( _defaults, tr( L"Tout remettre par défaut", L"Restore all defaults" ) );
            SetWindowTextW( _guide, tr( L"Guide d’équilibrage", L"Balance guide" ) );
            SetWindowTextW( _folder, tr( L"Dossier de configuration", L"Configuration folder" ) );
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
                SetWindowTextW( row.input, number( _values[i], setting.kind == km::Kind::Decimal && setting.id != std::string_view( "success" ) ).c_str() );
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

        void place( HWND item, const int x, const int y, const int width, const int height ) const
        {
            const auto rect = bounds( x, y, width, height );
            SetWindowPos( item, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW );
        }

        int textWidth( HDC dc, HWND item, HFONT font, const int padding ) const
        {
            const auto label = windowText( item );
            const auto previous = SelectObject( dc, font );
            SIZE size{};
            GetTextExtentPoint32W( dc, label.c_str(), static_cast<int>( label.size() ), &size );
            SelectObject( dc, previous );
            return dip( size.cx ) + padding;
        }

        void layout()
        {
            if ( !_panel || !_quit )
                return;
            RECT client{};
            GetClientRect( _window, &client );
            const int w = dip( client.right );
            const int h = dip( client.bottom );
            const int footerTop = h - footerHeight;

            const HDC dc = GetDC( _window );
            const int languageWidth = textWidth( dc, _language, _ui.body, 24 );
            place( _language, w - pageMargin - languageWidth, 16, languageWidth, 34 );

            int x = pageMargin;
            for ( const HWND tab : _tabs ) {
                const int width = textWidth( dc, tab, _ui.strong, 2 );
                place( tab, x, headerHeight, width, tabsHeight );
                x += width + 24;
            }
            x = pageMargin;
            for ( const HWND button : { _defaults, _guide, _readme, _folder } ) {
                const int width = textWidth( dc, button, _ui.body, 18 );
                place( button, x, footerTop + 12, width, 32 );
                x += width + 7;
            }
            x = w - pageMargin;
            for ( const HWND button : { _quit, _play, _save } ) {
                const int width = std::max( 69, textWidth( dc, button, _ui.strong, 24 ) );
                x -= width;
                place( button, x, footerTop + 56, width, 33 );
                x -= 7;
            }
            place( _status, pageMargin, footerTop + 56, std::max( 0, x - pageMargin - 7 ), 33 );
            ReleaseDC( _window, dc );

            const int top = headerHeight + tabsHeight;
            place( _panel, 0, top, w, std::max( 1, footerTop - top ) );
            layoutPanel();
            InvalidateRect( _window, nullptr, FALSE );
        }

        void layoutPanel()
        {
            RECT panel{};
            GetClientRect( _panel, &panel );
            const int panelWidth = dip( panel.right );
            const int panelHeight = std::max( 1, dip( panel.bottom ) );
            const int right = panelWidth - pageMargin;
            const int inputWidth = 156;
            const int inputX = right - inputWidth;
            const int resetX = inputX - 64;
            const int left = pageMargin;
            int y = 14 - _scroll;

            std::vector<HWND> visible;
            _fieldFrames.clear();
            _sectionRule = {};
            _sampleFrame = {};
            const auto put = [&]( HWND item, const int x, const int top, const int width, const int height ) {
                visible.push_back( item );
                place( item, x, top, width, height );
            };
            const auto heading = [&]( const size_t i, const int indent = 0 ) {
                put( _headings[i], left + indent, y, right - left - indent, 28 );
                y += 29;
            };
            const auto edit = [&]( HWND input, HWND spinner, const int x, const int top, const int width ) {
                _fieldFrames.push_back( bounds( x, top, width, 32 ) );
                const int arrowWidth = spinner ? 17 : 0;
                put( input, x + 5, top + 6, width - arrowWidth - 10, 20 );
                if ( spinner )
                    put( spinner, x + width - arrowWidth - 3, top + 4, arrowWidth, 24 );
            };
            const auto row = [&]( const size_t i, const int indent = 0 ) {
                const auto & entry = _rows[i];
                const auto kind = km::settings[i].kind;
                const bool hint = i == km::index( "cap" ) || i == km::index( "increment" );
                const int labelX = left + indent;
                put( entry.label, labelX, y, resetX - labelX - 10, hint ? 20 : 32 );
                if ( hint )
                    put( i == km::index( "cap" ) ? _capHint : _incrementHint, labelX, y + 18, resetX - labelX - 10, 16 );
                put( entry.reset, resetX, y + 3, 55, 26 );
                if ( kind == km::Kind::Toggle )
                    put( entry.input, inputX, y + 3, 24, 26 );
                else if ( kind == km::Kind::Choice )
                    put( entry.input, inputX, y + 1, inputWidth, 180 );
                else
                    edit( entry.input, entry.spin, inputX, y, inputWidth );
                y += hint ? 41 : rowHeight;
            };
            const auto named = [&]( const char * id, const int indent = 0 ) { row( km::index( id ), indent ); };
            const auto disclosure = [&]( HWND item ) {
                put( item, left, y + 5, right - left, 37 );
                y += 51;
            };

            switch ( _selectedTab ) {
            case 0:
                heading( 0 );
                named( "tax" );
                named( "cap" );
                _sectionRule = bounds( left, y + 2, right - left, 1 );
                y += 13;
                heading( 1 );
                named( "mode" );
                if ( const auto mode = _values[km::modeIndex]; mode != 0 ) {
                    named( "base" );
                    if ( mode == 1 ) {
                        named( "odd" );
                        named( "even" );
                    }
                    else {
                        for ( const auto id : { "increment", "step", "multiplier", "uniform" } )
                            named( id );
                        y += 4;
                        _sampleFrame = bounds( left, y, right - left, 48 );
                        put( _sampleLabel, left + 12, y + 8, 210, 32 );
                        edit( _sample, _sampleSpin, left + 224, y + 8, 96 );
                        put( _quote, left + 332, y + 8, right - left - 345, 32 );
                        y += 48;
                    }
                }
                break;
            case 1:
                heading( 2 );
                for ( const auto id : { "success", "archers", "pikemen", "swordsmen" } )
                    named( id );
                disclosure( _conversion );
                if ( _conversionOpen ) {
                    heading( 3, 12 );
                    for ( const auto id : { "tg0", "tg1", "tg2" } )
                        named( id, 12 );
                    y += 4;
                    heading( 4, 12 );
                    for ( const auto id : { "farm0", "farm1", "farm2" } )
                        named( id, 12 );
                }
                disclosure( _requirements );
                if ( _requirementsOpen ) {
                    for ( size_t i = 0; i < km::count; ++i ) {
                        if ( std::string_view( km::settings[i].key ).starts_with( "training_grounds.require_" ) )
                            row( i, 12 );
                    }
                }
                break;
            case 2:
                heading( 6 );
                named( "artwork" );
                y += 9;
                heading( 7 );
                put( _saveProfile, left, y, 167, 32 );
                put( _loadProfile, left + 175, y, 167, 32 );
                y += 36;
                break;
            }

            const auto hideUnused = [&]( HWND item ) {
                if ( item && std::find( visible.begin(), visible.end(), item ) == visible.end() )
                    ShowWindow( item, SW_HIDE );
            };
            for ( const auto & entry : _rows )
                for ( const auto item : { entry.label, entry.input, entry.reset, entry.spin } )
                    hideUnused( item );
            for ( const auto item : _headings )
                hideUnused( item );
            for ( const auto item : { _capHint, _incrementHint, _conversion, _requirements, _sampleLabel, _sample, _sampleSpin, _quote, _saveProfile, _loadProfile } )
                hideUnused( item );

            _contentHeight = y + _scroll + 14;
            _scrollMaximum = std::max( 0, _contentHeight - panelHeight );
            if ( _scroll > _scrollMaximum ) {
                _scroll = _scrollMaximum;
                layoutPanel();
                return;
            }
            SCROLLINFO scroll{ sizeof( SCROLLINFO ), SIF_RANGE | SIF_PAGE | SIF_POS, 0, _contentHeight - 1, static_cast<UINT>( panelHeight ), _scroll, 0 };
            SetScrollInfo( _panel, SB_VERT, &scroll, TRUE );
            RECT after{};
            GetClientRect( _panel, &after );
            if ( panel.right != after.right ) {
                layoutPanel();
                return;
            }
            InvalidateRect( _panel, nullptr, TRUE );
        }

        void fitToContent() const
        {
            if ( _userSized || IsZoomed( _window ) || IsIconic( _window ) )
                return;
            MONITORINFO monitor{};
            monitor.cbSize = sizeof( monitor );
            if ( !GetMonitorInfoW( MonitorFromWindow( _window, MONITOR_DEFAULTTONEAREST ), &monitor ) )
                return;
            RECT window{}, client{};
            GetWindowRect( _window, &window );
            GetClientRect( _window, &client );
            const auto height = std::min( monitor.rcWork.bottom - monitor.rcWork.top,
                                          window.bottom - window.top - client.bottom + px( headerHeight + tabsHeight + _contentHeight + footerHeight ) );
            if ( height == window.bottom - window.top )
                return;
            const auto top = std::clamp( window.top, monitor.rcWork.top, monitor.rcWork.bottom - height );
            SetWindowPos( _window, nullptr, window.left, top, window.right - window.left, height, SWP_NOZORDER | SWP_NOACTIVATE );
        }

        void revealFocusedField()
        {
            const HWND field = GetFocus();
            if ( !field || GetParent( field ) != _panel )
                return;
            RECT rect{}, client{};
            GetWindowRect( field, &rect );
            MapWindowPoints( nullptr, _panel, reinterpret_cast<POINT *>( &rect ), 2 );
            GetClientRect( _panel, &client );
            const auto delta = rect.top < 0 ? rect.top : std::max( 0L, rect.bottom - client.bottom );
            const int position = std::clamp( _scroll + dip( delta ), 0, _scrollMaximum );
            if ( position != _scroll ) {
                _scroll = position;
                layoutPanel();
            }
        }

        static void text( HDC dc, const wchar_t * label, RECT rect, HFONT font, const COLORREF color, const UINT alignment )
        {
            const int saved = SaveDC( dc );
            SelectObject( dc, font );
            SetBkMode( dc, TRANSPARENT );
            SetTextColor( dc, color );
            DrawTextW( dc, label, -1, &rect, alignment | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS );
            RestoreDC( dc, saved );
        }

        void paintMain()
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint( _window, &paint );
            RECT rect{};
            GetClientRect( _window, &rect );
            const int w = dip( rect.right );
            const int h = dip( rect.bottom );
            fill( dc, rect, white );
            fill( dc, bounds( 0, 0, w, headerHeight ), navy );
            fill( dc, bounds( 0, headerHeight, w, tabsHeight ), RGB( 251, 252, 254 ) );
            fill( dc, bounds( 0, headerHeight + tabsHeight - 1, w, 1 ), separator );
            fill( dc, bounds( 0, h - footerHeight, w, footerHeight ), footer );
            fill( dc, bounds( 0, h - footerHeight, w, 1 ), separator );
            if ( _ui.icon )
                DrawIconEx( dc, px( pageMargin ), px( 13 ), _ui.icon, px( 40 ), px( 40 ), 0, nullptr, DI_NORMAL );
            text( dc, L"Knight Mod Launcher", bounds( pageMargin + 52, 0, w - 280, headerHeight ), _ui.title, white, DT_LEFT );
            EndPaint( _window, &paint );
        }

        void paintPanel( HWND panel )
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint( panel, &paint );
            fill( dc, paint.rcPaint, white );
            if ( !IsRectEmpty( &_sectionRule ) )
                fill( dc, _sectionRule, separator );
            if ( !IsRectEmpty( &_sampleFrame ) )
                surface( dc, _sampleFrame, wash, wash, px( 12 ) );
            for ( const auto & frame : _fieldFrames )
                surface( dc, frame, white, edge, px( 10 ) );
            EndPaint( panel, &paint );
        }

        HBRUSH controlBrush( HDC dc, HWND control ) const
        {
            if ( control == _sampleLabel || control == _quote || control == _status ) {
                const bool status = control == _status;
                SetTextColor( dc, control == _quote ? blue : status ? muted : ink );
                SetBkColor( dc, status ? footer : wash );
                return status ? _ui.footerFill : _ui.callout;
            }
            SetTextColor( dc, control == _capHint || control == _incrementHint ? muted : ink );
            SetBkColor( dc, white );
            return _ui.paper;
        }

        bool drawItem( const DRAWITEMSTRUCT & item ) const
        {
            if ( item.CtlType == ODT_COMBOBOX ) {
                const bool popup = ( item.itemState & ODS_COMBOBOXEDIT ) == 0;
                const bool selected = popup && ( item.itemState & ODS_SELECTED ) != 0;
                fill( item.hDC, item.rcItem, selected ? wash : white );
                if ( item.itemID != static_cast<UINT>( -1 ) ) {
                    const auto length = SendMessageW( item.hwndItem, CB_GETLBTEXTLEN, item.itemID, 0 );
                    if ( length != CB_ERR ) {
                        std::wstring label( static_cast<size_t>( length ) + 1, L'\0' );
                        SendMessageW( item.hwndItem, CB_GETLBTEXT, item.itemID, reinterpret_cast<LPARAM>( label.data() ) );
                        auto rect = item.rcItem;
                        rect.left += px( 7 );
                        text( item.hDC, label.c_str(), rect, _ui.body, selected ? blue : ink, DT_LEFT );
                    }
                }
                if ( ( item.itemState & ( ODS_FOCUS | ODS_NOFOCUSRECT ) ) == ODS_FOCUS )
                    DrawFocusRect( item.hDC, &item.rcItem );
                return true;
            }
            if ( item.CtlType != ODT_BUTTON )
                return false;

            const auto id = item.CtlID;
            const bool focused = ( item.itemState & ( ODS_FOCUS | ODS_NOFOCUSRECT ) ) == ODS_FOCUS;
            const bool pressed = ( item.itemState & ODS_SELECTED ) != 0;
            const bool hot = item.hwndItem == _hotButton || pressed;
            auto rect = item.rcItem;
            const auto tab = std::find( _tabs.begin(), _tabs.end(), item.hwndItem );
            auto label = windowText( item.hwndItem );
            if ( tab != _tabs.end() ) {
                const bool active = static_cast<int>( tab - _tabs.begin() ) == _selectedTab;
                fill( item.hDC, rect, RGB( 251, 252, 254 ) );
                text( item.hDC, label.c_str(), rect, active ? _ui.strong : _ui.body, active || hot ? blue : muted, DT_LEFT );
                if ( active )
                    fill( item.hDC, { rect.left, rect.bottom - px( 2 ), rect.right, rect.bottom }, blue );
            }
            else {
                const bool language = id == languageId;
                const bool primary = id == playId;
                const bool reset = id >= firstReset && id < firstReset + km::count;
                const bool disclosure = id == conversionId || id == requirementsId;
                const bool panelButton = GetParent( item.hwndItem ) == _panel;
                fill( item.hDC, rect, language ? navy : panelButton ? white : footer );
                const COLORREF background = language ? ( hot ? RGB( 48, 73, 98 ) : navy )
                    : primary ? ( hot ? RGB( 27, 68, 122 ) : blue ) : hot ? wash : white;
                const COLORREF border = language ? RGB( 142, 166, 195 ) : primary ? background : disclosure ? separator : edge;
                surface( item.hDC, rect, background, focused ? blue : border, px( 10 ) );
                if ( disclosure ) {
                    label.insert( 0, ( id == conversionId ? _conversionOpen : _requirementsOpen ) ? L"▾  " : L"▸  " );
                    rect.left += px( 11 );
                }
                text( item.hDC, label.c_str(), rect, primary || disclosure ? _ui.strong : reset ? _ui.compactFont : _ui.body,
                      language || primary ? white : reset ? blue : ink, disclosure ? DT_LEFT : DT_CENTER );
            }
            if ( focused ) {
                rect = item.rcItem;
                InflateRect( &rect, -px( 3 ), -px( 3 ) );
                DrawFocusRect( item.hDC, &rect );
            }
            return true;
        }

        void status( const wchar_t * text )
        {
            SetWindowTextW( _status, text );
        }

        void refresh()
        {
            SetWindowTextW( _capHint, _values[km::index( "cap" )] == 0 ? tr( L"Illimité", L"Unlimited" ) : L"" );
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
            HWND hot{};
            size_t rowIndex = km::count;
            if ( GetForegroundWindow() == _window && GetCursorPos( &cursor ) ) {
                const HWND hovered = WindowFromPoint( cursor );
                if ( hovered ) {
                    if ( IsChild( _window, hovered ) && ( GetWindowLongPtrW( hovered, GWL_STYLE ) & BS_TYPEMASK ) == BS_OWNERDRAW )
                        hot = hovered;
                    for ( size_t i = 0; i < km::count; ++i ) {
                        const auto & row = _rows[i];
                        if ( hovered == row.label || hovered == row.input || hovered == row.reset || hovered == row.spin ) {
                            rowIndex = i;
                            break;
                        }
                    }
                }
            }
            if ( hot != _hotButton ) {
                if ( _hotButton )
                    InvalidateRect( _hotButton, nullptr, FALSE );
                _hotButton = hot;
                if ( hot )
                    InvalidateRect( hot, nullptr, FALSE );
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
            const double initial = km::defaultValue( rowIndex, _values );
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
            km::files::ensureDirectory( _config );
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
            km::files::ensureDirectory( _config );
            const std::string existing = km::files::isFile( _ini ) ? km::files::readText( _ini ) : "# Knight Mod settings\r\n";
            km::files::replaceText( _ini, km::updateIni( existing, _values ) );
            _saved = _values;
            status( tr( L"Réglages enregistrés.", L"Settings saved." ) );
            if ( play && !km::files::launchGame( _installation ) )
                error( L"Impossible de lancer fheroes2KM.exe. Vérifier que le lanceur et le jeu sont dans le même dossier.", L"Could not start fheroes2KM.exe. Place the launcher next to the game." );
        }

        void profile( const bool saveProfile )
        {
            if ( !capture() )
                return;
            const auto path = profilePath( saveProfile );
            if ( !path )
                return;
            if ( !km::files::isIniProfile( *path ) ) {
                error( L"Choisir un profil au format .ini.", L"Choose a profile in .ini format." );
                return;
            }
            if ( saveProfile ) {
                if ( km::files::sameFile( *path, _ini ) ) {
                    error( L"Choisis un nom différent de knight_mod.ini.", L"Choose a name other than knight_mod.ini." );
                    return;
                }
                km::files::replaceText( *path, km::updateIni( "# Knight Mod profile\r\n", _values ) );
                status( tr( L"Profil enregistré.", L"Profile saved." ) );
                return;
            }

            std::string contents;
            try {
                contents = km::files::readText( *path );
            }
            catch ( const km::files::FileTooLarge & ) {
                error( L"Le profil dépasse la taille maximale de 1 Mio.", L"The profile is larger than 1 MiB." );
                return;
            }
            if ( !km::load( contents, _values, true ) ) {
                error( L"Ce profil ne contient pas de réglages Knight Mod valides.", L"This profile does not contain valid Knight Mod settings." );
                return;
            }
            setControls();
            fitToContent();
            status( tr( L"Profil chargé. Enregistre ou lance le jeu.", L"Profile loaded. Save or play to apply." ) );
        }

        void open( const fs::path & path )
        {
            const auto result = reinterpret_cast<INT_PTR>( ShellExecuteW( _window, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL ) );
            if ( result <= 32 )
                error( L"Impossible d’ouvrir le fichier ou le dossier.", L"Could not open the file or folder." );
        }

        void openDocumentation( const wchar_t * frenchName, const wchar_t * englishName )
        {
            auto directory = _modDirectory / L"docs";
            if ( !km::files::isDirectory( directory ) )
                directory = _installation / L"docs" / L"Knight Mod";
            const auto english = directory / englishName;
            const auto french = directory / frenchName;
            if ( _french && km::files::isFile( french ) )
                open( french );
            else if ( km::files::isFile( english ) )
                open( english );
            else
                error( L"Document introuvable dans Knight Mod\\docs.", L"Document not found in Knight Mod\\docs." );
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
            km::files::ensureDirectory( _config );
            km::files::replaceText( _localeFile, _french ? "fr\n" : "en\n" );
        }

        void changeMode( const double mode )
        {
            if ( !km::selectMode( _values, mode ) )
                return;
            _updating = true;
            setField( km::modeIndex );
            if ( mode != 0 )
                setField( km::baseIndex );
            _updating = false;
            _scroll = 0;
            layout();
            fitToContent();
            refresh();
        }

        void command( const int id, const int notification )
        {
            if ( _updating )
                return;
            if ( id >= firstReset && id < firstReset + static_cast<int>( km::count ) && notification == BN_CLICKED ) {
                const size_t i = static_cast<size_t>( id - firstReset );
                const double initial = km::defaultValue( i, _values );
                if ( i == km::modeIndex ) {
                    changeMode( initial );
                    return;
                }
                _values[i] = initial;
                _updating = true;
                setField( i );
                _updating = false;
                refresh();
                return;
            }
            if ( id >= firstInput && id < firstInput + static_cast<int>( km::count ) ) {
                if ( notification != EN_CHANGE && notification != BN_CLICKED && notification != CBN_SELCHANGE )
                    return;
                const size_t i = static_cast<size_t>( id - firstInput );
                if ( const auto value = fieldValue( i ) ) {
                    if ( i == km::modeIndex ) {
                        changeMode( *value );
                        return;
                    }
                    _values[i] = *value;
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
                fitToContent();
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
                openDocumentation( L"Guide_de_Test_et_Equilibrage.pdf", L"Testing_and_Balance_Guide.pdf" );
                break;
            case readmeId:
                openDocumentation( L"Readme_FR.txt", L"Readme.txt" );
                break;
            case settingsTabId:
            case advancedTabId:
            case miscellaneousTabId:
                _selectedTab = id == settingsTabId ? 0 : id == advancedTabId ? 1 : 2;
                _scroll = 0;
                layoutPanel();
                fitToContent();
                for ( const auto tab : _tabs )
                    InvalidateRect( tab, nullptr, FALSE );
                break;
            case conversionId:
                _conversionOpen = !_conversionOpen;
                layoutPanel();
                fitToContent();
                InvalidateRect( _conversion, nullptr, FALSE );
                break;
            case requirementsId:
                _requirementsOpen = !_requirementsOpen;
                layoutPanel();
                fitToContent();
                InvalidateRect( _requirements, nullptr, FALSE );
                break;
            case folderId:
                km::files::ensureDirectory( _config );
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
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT:
                paintMain();
                return 0;
            case WM_DRAWITEM:
                if ( const auto * item = reinterpret_cast<const DRAWITEMSTRUCT *>( lParam ); item && drawItem( *item ) )
                    return TRUE;
                break;
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLOREDIT:
                return reinterpret_cast<LRESULT>( controlBrush( reinterpret_cast<HDC>( wParam ), reinterpret_cast<HWND>( lParam ) ) );
            case WM_CREATE:
                create();
                return 0;
            case WM_SIZE:
                if ( wParam != SIZE_MINIMIZED )
                    layout();
                return 0;
            case WM_SIZING:
                _userSized = true;
                break;
            case WM_GETMINMAXINFO: {
                auto * info = reinterpret_cast<MINMAXINFO *>( lParam );
                RECT work{}, minimum{ 0, 0, px( minWidth ), px( 330 ) };
                SystemParametersInfoW( SPI_GETWORKAREA, 0, &work, 0 );
                AdjustWindowRectEx( &minimum, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT );
                info->ptMinTrackSize = { std::min( minimum.right - minimum.left, work.right - work.left ),
                                         std::min( minimum.bottom - minimum.top, work.bottom - work.top ) };
                return 0;
            }
            case WM_COMMAND:
                command( LOWORD( wParam ), HIWORD( wParam ) );
                return 0;
            case WM_TIMER:
                updateTooltip();
                return 0;
            case WM_MOUSEWHEEL:
                _scroll = std::clamp( _scroll - GET_WHEEL_DELTA_WPARAM( wParam ) * rowHeight * 3 / WHEEL_DELTA, 0, _scrollMaximum );
                layoutPanel();
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
                layoutPanel();
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
        fs::path _modDirectory;
        fs::path _config;
        fs::path _ini;
        fs::path _localeFile;
        std::array<Row, km::count> _rows{};
        std::array<HWND, 8> _headings{};
        std::array<HWND, 3> _tabs{};
        km::Values _values{};
        km::Values _saved{};
        UiResources _ui;
        std::vector<RECT> _fieldFrames;
        RECT _sectionRule{}, _sampleFrame{};
        HWND _window{}, _panel{}, _tooltip{}, _language{}, _sample{}, _sampleLabel{}, _sampleSpin{}, _quote{};
        HWND _defaults{}, _save{}, _play{}, _quit{}, _guide{}, _readme{}, _folder{}, _saveProfile{}, _loadProfile{}, _status{};
        HWND _capHint{}, _incrementHint{}, _conversion{}, _requirements{}, _hotButton{};
        int _dpi{ 96 }, _selectedTab{}, _scroll{}, _scrollMaximum{}, _contentHeight{};
        size_t _hoveredRow = km::count;
        ULONGLONG _hoverStarted{};
        bool _french{}, _updating{}, _showingTooltip{}, _conversionOpen{}, _requirementsOpen{}, _userSized{};
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
