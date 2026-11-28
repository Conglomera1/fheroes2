// SPDX-License-Identifier: GPL-2.0-or-later
#include "documented_ini.h"
#include "settings_files.h"

#include <wx/collpane.h>
#include <wx/filedlg.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/tipwin.h>
#include <wx/tokenzr.h>
#include <wx/wx.h>
#include <wx/msw/dc.h>
#include <wx/msw/wrapwin.h>

#include <array>
#include <initializer_list>
#include <utility>

namespace
{
    namespace fs = std::filesystem;
    namespace storage = km::files;

    const wxColour navy( 23, 44, 71 );
    const wxColour blue( 36, 83, 142 );
    const wxColour ink( 23, 36, 54 );
    const wxColour muted( 77, 93, 113 );
    const wxColour pale( 245, 248, 252 );
    const wxColour wash( 238, 244, 251 );
    const wxColour border( 174, 190, 209 );
    const wxColour rule( 217, 225, 233 );
    constexpr int firstFieldId = 200;
    constexpr int firstTabId = 310;
    constexpr int statusId = 319;

    enum class ButtonStyle { Regular, Header, Primary, Tab };

    class LauncherButton final : public wxButton
    {
    public:
        LauncherButton( wxWindow * parent, const int id, const wxString & label, const ButtonStyle style )
            : wxButton( parent, id, label, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT )
            , _style( style )
        {
            MakeOwnerDrawn();
        }

        void select( const bool selected )
        {
            _selected = selected;
            Refresh();
        }

    protected:
        wxSize DoGetBestSize() const override
        {
            const int padding = _style == ButtonStyle::Tab ? 24 : 20;
            return wxSize( GetTextExtent( GetLabel() ).x + FromDIP( padding ), FromDIP( _style == ButtonStyle::Tab ? 40 : 30 ) );
        }

        bool MSWOnDraw( WXDRAWITEMSTRUCT * item ) override
        {
            const auto & native = *reinterpret_cast<const DRAWITEMSTRUCT *>( item );
            const wxRect bounds( native.rcItem.left, native.rcItem.top, native.rcItem.right - native.rcItem.left, native.rcItem.bottom - native.rcItem.top );
            wxDCTemp dc( native.hDC, bounds.GetSize() );
            const bool down = ( native.itemState & ODS_SELECTED ) != 0;
            const bool focused = ( native.itemState & ODS_FOCUS ) != 0;
            const bool primary = _style == ButtonStyle::Primary;
            const bool header = _style == ButtonStyle::Header;
            const auto surface = GetParent()->GetBackgroundColour();
            dc.SetPen( *wxTRANSPARENT_PEN );
            dc.SetBrush( wxBrush( surface ) );
            dc.DrawRectangle( bounds );
            if ( _style == ButtonStyle::Tab ) {
                if ( _selected ) {
                    dc.SetBrush( wxBrush( blue ) );
                    dc.DrawRectangle( bounds.x + FromDIP( 8 ), bounds.GetBottom() - FromDIP( 2 ), bounds.width - FromDIP( 16 ), FromDIP( 2 ) );
                }
            }
            else {
                dc.SetPen( wxPen( primary ? blue : border ) );
                dc.SetBrush( wxBrush( primary ? ( down ? navy : blue ) : header ? ( down ? blue : navy ) : ( down ? wash : *wxWHITE ) ) );
                dc.DrawRoundedRectangle( wxRect( bounds ).Deflate( 1 ), FromDIP( 5 ) );
            }
            if ( focused && ( native.itemState & ODS_NOFOCUSRECT ) == 0 ) {
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.SetPen( wxPen( primary || header ? *wxWHITE : blue, 1, wxPENSTYLE_DOT ) );
                dc.DrawRoundedRectangle( wxRect( bounds ).Deflate( FromDIP( 3 ) ), FromDIP( 4 ) );
            }
            dc.SetFont( primary || _selected ? GetFont().Bold() : GetFont() );
            dc.SetTextForeground( primary || header ? *wxWHITE : _selected ? blue : ink );
            dc.DrawLabel( GetLabel(), bounds, wxALIGN_CENTER );
            return true;
        }

    private:
        ButtonStyle _style;
        bool _selected = false;
    };

    struct SettingRow
    {
        wxPanel * panel{};
        wxStaticText * label{};
        wxStaticText * hint{};
        wxButton * reset{};
        wxWindow * input{};
    };

    class SettingsFrame final : public wxFrame
    {
    public:
        explicit SettingsFrame( const wxString & language )
            : wxFrame( nullptr, wxID_ANY, L"Knight Mod Launcher" )
            , _installation( storage::installationDirectory() )
            , _mod( _installation / L"Knight Mod" )
            , _configuration( storage::configurationDirectory() )
            , _ini( _configuration / L"knight_mod.ini" )
            , _languageFile( _configuration / L"knight_mod_launcher.language" )
            , _helpTimer( this )
        {
            _french = wxLocale::GetSystemLanguage() == wxLANGUAGE_FRENCH
                      || wxLocale::GetLanguageCanonicalName( wxLocale::GetSystemLanguage() ).StartsWith( L"fr" );
            for ( const auto & candidate : { _languageFile, _mod / L"KnightModLauncher.language", _installation / L"KnightModLauncher.language" } ) {
                if ( storage::isFile( candidate ) ) {
                    _french = km::trim( storage::readText( candidate ) ) == "fr";
                    break;
                }
            }
            if ( !language.empty() )
                _french = language == L"fr";
            if ( storage::isFile( _ini ) )
                km::load( storage::readText( _ini ), _values, false );
            _saved = _values;

            const wxIcon icon( L"KMSettingsIcon", wxBITMAP_TYPE_ICO_RESOURCE );
            if ( icon.IsOk() )
                SetIcon( icon );
            auto * root = new wxBoxSizer( wxVERTICAL );
            auto * background = new wxPanel( this );
            background->SetForegroundColour( ink );
            background->SetBackgroundColour( *wxWHITE );
            auto * body = new wxBoxSizer( wxVERTICAL );
            background->SetSizer( body );
            root->Add( background, 1, wxEXPAND );
            SetSizer( root );
            addHeader( background, body, icon );
            addPages( background, body );
            addFooter( background, body );

            Bind( wxEVT_TIMER, [this]( wxTimerEvent & ) { showHelp(); }, _helpTimer.GetId() );
            Bind( wxEVT_CLOSE_WINDOW, [this]( wxCloseEvent & event ) {
                clearHelp();
                event.Skip();
            } );
            localize();
            fitPage();
            Centre();
        }

        ~SettingsFrame() override { clearHelp(); }

    private:
        const wchar_t * tr( const wchar_t * fr, const wchar_t * en ) const { return _french ? fr : en; }
        int dip( const int size ) const { return FromDIP( size ); }

        wxString number( const double value, const bool showDecimal = false ) const
        {
            wxString formatted = wxString::FromUTF8( km::format( value ).c_str() );
            if ( showDecimal && !formatted.Contains( L"." ) )
                formatted += L".0";
            if ( _french )
                formatted.Replace( L".", L"," );
            return formatted;
        }

        wxStaticText * text( wxWindow * parent, const wxString & label = {}, const bool bold = false, const int id = wxID_ANY ) const
        {
            auto * item = new wxStaticText( parent, id, label );
            if ( bold )
                item->SetFont( item->GetFont().Bold() );
            return item;
        }

        template <typename Handler>
        LauncherButton * button( wxWindow * parent, const wxString & label, Handler handler, const int id = wxID_ANY,
                                 const ButtonStyle style = ButtonStyle::Regular )
        {
            auto * item = new LauncherButton( parent, id, label, style );
            item->Bind( wxEVT_BUTTON, [this, handler]( wxCommandEvent & ) {
                clearHelp();
                try {
                    handler();
                }
                catch ( const std::exception & ) {
                    error( L"L’opération a échoué.", L"The operation failed." );
                }
            } );
            return item;
        }

        void addHeader( wxWindow * parent, wxBoxSizer * body, const wxIcon & icon )
        {
            auto * header = new wxPanel( parent );
            header->SetBackgroundColour( navy );
            auto * line = new wxBoxSizer( wxHORIZONTAL );
            if ( icon.IsOk() )
                line->Add( new wxStaticBitmap( header, wxID_ANY, wxBitmap( icon ) ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip( 12 ) );
            auto * title = text( header, L"Knight Mod Launcher", true );
            title->SetFont( title->GetFont().Larger() );
            title->SetForegroundColour( *wxWHITE );
            line->Add( title, 1, wxALIGN_CENTER_VERTICAL );
            line->Add( button( header, L"Change language", [this] { changeLanguage(); }, wxID_ANY, ButtonStyle::Header ), 0, wxALIGN_CENTER_VERTICAL );
            auto * margins = new wxBoxSizer( wxVERTICAL );
            margins->Add( line, 1, wxEXPAND | wxALL, dip( 16 ) );
            header->SetSizer( margins );
            body->Add( header, 0, wxEXPAND );
        }

        void addPages( wxWindow * parent, wxBoxSizer * body )
        {
            auto * tabs = new wxPanel( parent );
            tabs->SetBackgroundColour( pale );
            auto * tabLine = new wxBoxSizer( wxHORIZONTAL );
            for ( size_t i = 0; i < _pages.size(); ++i ) {
                _tabs[i] = button( tabs, L"", [this, i] { selectPage( i ); }, firstTabId + static_cast<int>( i ), ButtonStyle::Tab );
                tabLine->Add( _tabs[i], 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip( 7 ) );
            }
            auto * tabMargins = new wxBoxSizer( wxVERTICAL );
            tabMargins->Add( tabLine, 0, wxEXPAND | wxLEFT, dip( 14 ) );
            tabs->SetSizer( tabMargins );
            body->Add( tabs, 0, wxEXPAND );

            _book = new wxSimplebook( parent, wxID_ANY );
            _book->SetMinSize( wxSize( -1, dip( 100 ) ) );
            for ( auto & page : _pages ) {
                page = new wxScrolledWindow( _book, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxTAB_TRAVERSAL );
                page->SetBackgroundColour( *wxWHITE );
                page->SetScrollRate( 0, dip( 10 ) );
                _book->AddPage( page, wxString() );
                page->SetSizer( new wxBoxSizer( wxVERTICAL ) );
            }
            _book->ChangeSelection( 0 );
            body->Add( _book, 1, wxEXPAND );
            makeSettingsPage();
            makeAdvancedPage();
            makeMiscPage();
        }

        void appendRows( wxWindow * parent, wxSizer * column, const std::initializer_list<const char *> ids )
        {
            for ( const auto id : ids )
                appendRow( parent, column, km::index( id ) );
        }

        void appendRow( wxWindow * parent, wxSizer * column, const size_t index )
        {
            const auto & setting = km::settings[index];
            auto & row = _rows[index];
            const int id = firstFieldId + static_cast<int>( index );
            row.panel = new wxPanel( parent );
            auto * line = new wxBoxSizer( wxHORIZONTAL );
            auto * labels = new wxBoxSizer( wxVERTICAL );
            row.label = text( row.panel );
            labels->Add( row.label );
            if ( index == km::index( "cap" ) || index == km::index( "increment" ) ) {
                row.hint = text( row.panel );
                row.hint->SetForegroundColour( muted );
                row.hint->SetFont( row.hint->GetFont().Smaller() );
                labels->Add( row.hint );
            }
            line->Add( labels, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip( 8 ) );
            row.reset = button( row.panel, L"", [this, index] { resetSetting( index ); } );
            line->Add( row.reset, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip( 8 ) );

            const wxSize fieldSize( dip( 150 ), -1 );
            switch ( setting.kind ) {
            case km::Kind::Choice:
                row.input = new wxChoice( row.panel, id, wxDefaultPosition, fieldSize );
                row.input->Bind( wxEVT_CHOICE, [this, index]( wxCommandEvent & ) { changed( index ); } );
                break;
            case km::Kind::Toggle:
                row.input = new wxCheckBox( row.panel, id, L"" );
                row.input->Bind( wxEVT_CHECKBOX, [this, index]( wxCommandEvent & ) { changed( index ); } );
                break;
            case km::Kind::Integer:
                row.input = new wxSpinCtrl( row.panel, id, wxString(), wxDefaultPosition, fieldSize, wxSP_ARROW_KEYS | wxALIGN_RIGHT,
                                            static_cast<int>( setting.minimum ), static_cast<int>( setting.maximum ) );
                row.input->Bind( wxEVT_SPINCTRL, [this, index]( wxSpinEvent & ) { changed( index ); } );
                row.input->Bind( wxEVT_TEXT, [this, index]( wxCommandEvent & ) { changed( index ); } );
                break;
            case km::Kind::Decimal:
                row.input = new wxTextCtrl( row.panel, id, wxString(), wxDefaultPosition, fieldSize, wxTE_RIGHT );
                row.input->Bind( wxEVT_TEXT, [this, index]( wxCommandEvent & ) { changed( index ); } );
                break;
            }
            auto * field = new wxBoxSizer( wxHORIZONTAL );
            field->SetMinSize( fieldSize );
            field->Add( row.input, 0, wxALIGN_CENTER_VERTICAL );
            line->Add( field, 0, wxALIGN_CENTER_VERTICAL );
            row.panel->SetSizer( line );
            column->Add( row.panel, 0, wxEXPAND | wxBOTTOM, dip( 8 ) );
            for ( auto * helpTarget : { static_cast<wxWindow *>( row.label ), static_cast<wxWindow *>( row.reset ), row.input } ) {
                helpTarget->Bind( wxEVT_ENTER_WINDOW, [this, index, helpTarget]( wxMouseEvent & event ) {
                    clearHelp();
                    _helpSetting = index;
                    _helpAnchor = helpTarget;
                    _helpTimer.StartOnce( 550 );
                    event.Skip();
                } );
                helpTarget->Bind( wxEVT_LEAVE_WINDOW, [this]( wxMouseEvent & event ) {
                    clearHelp();
                    event.Skip();
                } );
            }
        }

        wxBoxSizer * content( wxScrolledWindow * page )
        {
            auto * column = new wxBoxSizer( wxVERTICAL );
            page->GetSizer()->Add( column, 0, wxEXPAND | wxALL, dip( 20 ) );
            return column;
        }

        void makeSettingsPage()
        {
            auto * page = _pages[0];
            auto * column = content( page );
            column->Add( text( page, L"Taxation", true ), 0, wxBOTTOM, dip( 9 ) );
            appendRows( page, column, { "tax", "cap" } );
            auto * divider = new wxPanel( page, wxID_ANY, wxDefaultPosition, wxSize( -1, dip( 1 ) ) );
            divider->SetBackgroundColour( rule );
            column->Add( divider, 0, wxEXPAND | wxTOP | wxBOTTOM, dip( 8 ) );
            column->Add( text( page, L"Conscription", true ), 0, wxBOTTOM, dip( 9 ) );
            appendRows( page, column, { "mode", "base", "odd", "even", "increment", "step", "multiplier", "uniform" } );

            _example = new wxPanel( page );
            _example->SetBackgroundColour( wash );
            auto * example = new wxBoxSizer( wxHORIZONTAL );
            _exampleLabel = text( _example );
            _quantity = new wxSpinCtrl( _example, 309, L"3", wxDefaultPosition, wxSize( dip( 82 ), -1 ),
                                        wxSP_ARROW_KEYS, 0, static_cast<int>( Conscription::maxDailyOrder ), 3 );
            _quote = text( _example, L"", true );
            _quote->SetForegroundColour( blue );
            _quantity->Bind( wxEVT_TEXT, [this]( wxCommandEvent & ) { refreshQuote(); } );
            _quantity->Bind( wxEVT_SPINCTRL, [this]( wxSpinEvent & ) { refreshQuote(); } );
            example->Add( _exampleLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, dip( 9 ) );
            example->Add( _quantity, 0, wxALIGN_CENTER_VERTICAL );
            example->AddStretchSpacer();
            example->Add( _quote, 0, wxALIGN_CENTER_VERTICAL | wxALL, dip( 9 ) );
            _example->SetSizer( example );
            column->Add( _example, 0, wxEXPAND | wxTOP, dip( 5 ) );
        }

        void makeAdvancedPage()
        {
            auto * page = _pages[1];
            auto * column = content( page );
            _training = text( page, L"", true );
            column->Add( _training, 0, wxBOTTOM, dip( 9 ) );
            appendRows( page, column, { "success", "archers", "pikemen", "swordsmen" } );
            _conversion = new wxCollapsiblePane( page, 316, L"", wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE );
            auto * costPanel = _conversion->GetPane();
            auto * costs = new wxBoxSizer( wxVERTICAL );
            _convert = text( costPanel, L"", true );
            costs->Add( _convert, 0, wxTOP | wxBOTTOM, dip( 8 ) );
            appendRows( costPanel, costs, { "tg0", "tg1", "tg2" } );
            _restore = text( costPanel, L"", true );
            costs->Add( _restore, 0, wxTOP | wxBOTTOM, dip( 8 ) );
            appendRows( costPanel, costs, { "farm0", "farm1", "farm2" } );
            costPanel->SetSizer( costs );
            column->Add( _conversion, 0, wxEXPAND | wxTOP | wxBOTTOM, dip( 6 ) );

            _requirements = new wxCollapsiblePane( page, 317, L"", wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE );
            auto * requirementsPanel = _requirements->GetPane();
            auto * requirements = new wxBoxSizer( wxVERTICAL );
            for ( size_t i = 0; i < km::count; ++i ) {
                if ( std::string_view( km::settings[i].key ).starts_with( "training_grounds.require_" ) )
                    appendRow( requirementsPanel, requirements, i );
            }
            requirementsPanel->SetSizer( requirements );
            column->Add( _requirements, 0, wxEXPAND | wxTOP, dip( 6 ) );
            for ( auto * pane : { _conversion, _requirements } )
                pane->Bind( wxEVT_COLLAPSIBLEPANE_CHANGED, [this]( wxCollapsiblePaneEvent & ) { fitPage(); } );
        }

        void makeMiscPage()
        {
            auto * page = _pages[2];
            auto * column = content( page );
            _appearance = text( page, L"", true );
            column->Add( _appearance, 0, wxBOTTOM, dip( 9 ) );
            appendRows( page, column, { "artwork" } );
            _profiles = text( page, L"", true );
            column->Add( _profiles, 0, wxTOP | wxBOTTOM, dip( 8 ) );
            auto * actions = new wxBoxSizer( wxHORIZONTAL );
            _saveProfile = button( page, L"", [this] { profile( true ); }, 307 );
            _loadProfile = button( page, L"", [this] { profile( false ); }, 308 );
            actions->Add( _saveProfile, 0, wxRIGHT, dip( 8 ) );
            actions->Add( _loadProfile );
            column->Add( actions );
        }

        void addFooter( wxWindow * parent, wxBoxSizer * body )
        {
            auto * footer = new wxPanel( parent );
            footer->SetBackgroundColour( pale );
            auto * footerSizer = new wxBoxSizer( wxVERTICAL );
            auto * commands = new wxBoxSizer( wxHORIZONTAL );
            _defaults = button( footer, L"", [this] {
                _values = km::defaults();
                showValues();
                fitPage();
            }, 306 );
            _guide = button( footer, L"", [this] { openDocument( L"Guide_de_Test_et_Equilibrage.pdf", L"Testing_and_Balance_Guide.pdf" ); }, 301 );
            auto * readme = button( footer, L"Readme", [this] { openDocument( L"Readme_FR.txt", L"Readme.txt" ); }, 315 );
            _folder = button( footer, L"", [this] {
                storage::ensureDirectory( _configuration );
                openPath( _configuration );
            }, 302 );
            for ( auto * item : std::array<wxButton *, 4>{ _defaults, _guide, readme, _folder } )
                commands->Add( item, 0, wxRIGHT, dip( 7 ) );
            footerSizer->Add( commands, 0, wxEXPAND | wxALL, dip( 10 ) );

            auto * actions = new wxBoxSizer( wxHORIZONTAL );
            _status = text( footer, L"", false, statusId );
            _status->SetForegroundColour( muted );
            actions->Add( _status, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip( 6 ) );
            _save = button( footer, L"", [this] { save( false ); }, 303 );
            _play = button( footer, L"", [this] { save( true ); }, 304, ButtonStyle::Primary );
            _quit = button( footer, L"", [this] { Close(); }, 305 );
            for ( auto * item : { _save, _play, _quit } )
                actions->Add( item, 0, wxLEFT, dip( 6 ) );
            footerSizer->Add( actions, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dip( 10 ) );
            footer->SetSizer( footerSizer );
            body->Add( footer, 0, wxEXPAND );
        }

        void localize()
        {
            _synchronizing = true;
            _tabs[0]->SetLabel( tr( L"Réglages", L"Settings" ) );
            _tabs[1]->SetLabel( tr( L"Avancé", L"Advanced" ) );
            _tabs[2]->SetLabel( tr( L"Divers", L"Miscellaneous" ) );
            for ( size_t i = 0; i < km::count; ++i ) {
                const auto & setting = km::settings[i];
                const auto & row = _rows[i];
                row.label->SetLabel( tr( setting.french, setting.english ) );
                row.reset->SetLabel( tr( L"Défaut", L"Default" ) );
                if ( setting.kind == km::Kind::Choice ) {
                    auto * choice = static_cast<wxChoice *>( row.input );
                    choice->Clear();
                    wxStringTokenizer options( tr( setting.frenchChoices, setting.englishChoices ), L"|" );
                    while ( options.HasMoreTokens() )
                        choice->Append( options.GetNextToken() );
                }
            }
            _training->SetLabel( tr( L"Formation", L"Training" ) );
            _conversion->SetLabel( tr( L"Coûts de conversion", L"Conversion costs" ) );
            _convert->SetLabel( tr( L"Passage aux terrains d’entraînement", L"Convert to Training Grounds" ) );
            _restore->SetLabel( tr( L"Retour à la Ferme", L"Restore the Farm" ) );
            _requirements->SetLabel( tr( L"Prérequis des terrains d’entraînement", L"Training Grounds requirements" ) );
            _appearance->SetLabel( tr( L"Affichage", L"Display" ) );
            _profiles->SetLabel( tr( L"Profils", L"Profiles" ) );
            _saveProfile->SetLabel( tr( L"Enregistrer un profil", L"Save Profile" ) );
            _loadProfile->SetLabel( tr( L"Charger un profil", L"Load Profile" ) );
            _exampleLabel->SetLabel( tr( L"Paysans de garnison (exemple)", L"Garrison Peasants (example)" ) );
            _rows[km::index( "increment" )].hint->SetLabel( tr( L"1,0 = le coût de base", L"1.0 = the base cost" ) );
            _defaults->SetLabel( tr( L"Tout remettre par défaut", L"Restore all defaults" ) );
            _guide->SetLabel( tr( L"Guide d’équilibrage", L"Balance guide" ) );
            _folder->SetLabel( tr( L"Dossier de configuration", L"Configuration folder" ) );
            _save->SetLabel( tr( L"Enregistrer", L"Save" ) );
            _play->SetLabel( tr( L"Jouer", L"Play" ) );
            _quit->SetLabel( tr( L"Quitter", L"Quit" ) );
            _synchronizing = false;
            showValues();
        }

        void writeControl( const size_t i )
        {
            const auto & setting = km::settings[i];
            auto * input = _rows[i].input;
            switch ( setting.kind ) {
            case km::Kind::Toggle:
                static_cast<wxCheckBox *>( input )->SetValue( _values[i] != 0 );
                break;
            case km::Kind::Choice:
                static_cast<wxChoice *>( input )->SetSelection( static_cast<int>( _values[i] ) );
                break;
            case km::Kind::Integer:
                static_cast<wxSpinCtrl *>( input )->SetValue( static_cast<int>( _values[i] ) );
                break;
            case km::Kind::Decimal:
                static_cast<wxTextCtrl *>( input )->ChangeValue( number( _values[i], setting.id != std::string_view( "success" ) ) );
                break;
            }
        }

        std::optional<double> readControl( const size_t i ) const
        {
            auto * input = _rows[i].input;
            const auto & setting = km::settings[i];
            if ( setting.kind == km::Kind::Toggle )
                return static_cast<wxCheckBox *>( input )->GetValue() ? 1.0 : 0.0;
            if ( setting.kind == km::Kind::Choice ) {
                const int choice = static_cast<wxChoice *>( input )->GetSelection();
                if ( choice == wxNOT_FOUND )
                    return {};
                return choice;
            }
            wxString entry = setting.kind == km::Kind::Integer ? static_cast<wxSpinCtrl *>( input )->GetTextValue()
                                                               : static_cast<wxTextCtrl *>( input )->GetValue();
            return km::parse( entry.Trim().Trim( false ).ToStdString(), setting );
        }

        void showValues()
        {
            _synchronizing = true;
            for ( size_t i = 0; i < km::count; ++i )
                writeControl( i );
            _synchronizing = false;
            applyMode();
            refresh();
        }

        void applyMode()
        {
            const auto mode = _values[km::modeIndex];
            _rows[km::baseIndex].panel->Show( mode != 0 );
            for ( const auto id : { "odd", "even" } )
                _rows[km::index( id )].panel->Show( mode == 1 );
            for ( const auto id : { "increment", "step", "multiplier", "uniform" } )
                _rows[km::index( id )].panel->Show( mode == 2 );
            _example->Show( mode == 2 );
        }

        void changed( const size_t index )
        {
            if ( _synchronizing )
                return;
            if ( const auto value = readControl( index ) ) {
                if ( index == km::modeIndex && km::selectMode( _values, *value ) ) {
                    _synchronizing = true;
                    writeControl( km::baseIndex );
                    _synchronizing = false;
                    applyMode();
                    fitPage();
                }
                else
                    _values[index] = *value;
            }
            refresh();
        }

        void resetSetting( const size_t index )
        {
            const auto initial = km::defaultValue( index, _values );
            if ( index == km::modeIndex ) {
                km::selectMode( _values, initial );
                showValues();
                fitPage();
                return;
            }
            _values[index] = initial;
            _synchronizing = true;
            writeControl( index );
            _synchronizing = false;
            refresh();
        }

        bool capture()
        {
            km::Values values = _values;
            for ( size_t i = 0; i < km::count; ++i ) {
                if ( const auto value = readControl( i ) )
                    values[i] = *value;
                else if ( _rows[i].panel->IsShown() ) {
                    wxMessageBox( tr( L"Vérifier la valeur de « ", L"Check the value for “" ) + _rows[i].label->GetLabel()
                                      + tr( L" ».", L"”." ),
                                  tr( L"Valeur incorrecte", L"Invalid value" ), wxOK | wxICON_ERROR, this );
                    return false;
                }
            }
            _values = values;
            return true;
        }

        void refreshQuote()
        {
            unsigned long quantity = 0;
            const bool valid = _quantity->GetTextValue().ToULong( &quantity ) && quantity <= Conscription::maxDailyOrder
                               && readControl( km::baseIndex ) && readControl( km::index( "increment" ) ) && readControl( km::index( "step" ) )
                               && readControl( km::index( "multiplier" ) );
            _quote->SetLabel( valid ? wxString::Format( L"%llu", static_cast<unsigned long long>( km::quote( _values, static_cast<uint32_t>( quantity ) ) ) )
                                          + tr( L" or", L" gold" )
                                    : wxString( L"—" ) );
            _example->Layout();
        }

        void refresh()
        {
            _rows[km::index( "cap" )].hint->SetLabel( _values[km::index( "cap" )] == 0 ? tr( L"Illimité", L"Unlimited" ) : L"" );
            refreshQuote();
            _status->SetLabel( _values == _saved ? L"" : tr( L"Modifications non enregistrées", L"Unsaved changes" ) );
        }

        void selectPage( const size_t page )
        {
            _book->ChangeSelection( page );
            fitPage();
        }

        void fitPage()
        {
            const auto selected = static_cast<size_t>( _book->GetSelection() );
            auto * page = _pages[selected];
            for ( size_t i = 0; i < _tabs.size(); ++i )
                _tabs[i]->select( i == selected );
            page->Layout();
            page->FitInside();
            const int chrome = GetSizer()->GetMinSize().y - _book->GetMinSize().y;
            const int natural = chrome + page->GetSizer()->GetMinSize().y;
            const wxRect workArea = wxGetClientDisplayRect();
            const int targetWidth = std::max( dip( 648 ), GetSizer()->GetMinSize().x );
            SetMinClientSize( wxSize( std::min( targetWidth, workArea.width - dip( 24 ) ), chrome + dip( 70 ) ) );
            if ( !IsMaximized() ) {
                const int width = std::min( std::max( targetWidth, GetClientSize().x ), workArea.width - dip( 24 ) );
                SetClientSize( width, std::min( natural, workArea.height - dip( 50 ) ) );
            }
            Layout();
        }

        void showHelp()
        {
            if ( !_helpAnchor || !_helpAnchor->IsShownOnScreen() )
                return;
            wxRect bounds = _helpAnchor->GetScreenRect();
            if ( !bounds.Contains( wxGetMousePosition() ) )
                return;
            const auto & setting = km::settings[_helpSetting];
            wxString caption = tr( setting.frenchHelp, setting.englishHelp );
            caption += tr( L"\n\nPar défaut : ", L"\n\nDefault: " );
            const double value = km::defaultValue( _helpSetting, _values );
            if ( setting.kind == km::Kind::Choice ) {
                const auto * choice = static_cast<wxChoice *>( _rows[_helpSetting].input );
                caption += choice->GetString( static_cast<unsigned int>( value ) );
            }
            else if ( setting.kind == km::Kind::Toggle )
                caption += value == 0 ? tr( L"Non", L"No" ) : tr( L"Oui", L"Yes" );
            else
                caption += number( value, setting.kind == km::Kind::Decimal );
            _tip = new wxTipWindow( this, caption, dip( 400 ), &_tip, &bounds );
        }

        void clearHelp()
        {
            _helpTimer.Stop();
            _helpAnchor = nullptr;
            if ( auto * tip = std::exchange( _tip, nullptr ) ) {
                tip->SetTipWindowPtr( nullptr );
                tip->Close();
            }
        }

        void error( const wchar_t * fr, const wchar_t * en )
        {
            wxMessageBox( tr( fr, en ), GetTitle(), wxOK | wxICON_ERROR, this );
        }

        void save( const bool play )
        {
            if ( !capture() )
                return;
            storage::ensureDirectory( _configuration );
            const auto existing = storage::isFile( _ini ) ? storage::readText( _ini ) : std::string();
            const auto updated = km::updateDocumentedIni( existing, _values );
            if ( updated.size() > storage::maxProfileSize )
                throw storage::FileTooLarge();
            storage::replaceText( _ini, updated );
            _saved = _values;
            _status->SetLabel( tr( L"Réglages enregistrés.", L"Settings saved." ) );
            if ( play && !storage::launchGame( _installation ) )
                error( L"Impossible de lancer fheroes2KM.exe. Vérifier que le lanceur et le jeu sont dans le même dossier.",
                       L"Could not start fheroes2KM.exe. Place the launcher next to the game." );
        }

        void profile( const bool writing )
        {
            if ( !capture() )
                return;
            storage::ensureDirectory( _configuration );
            wxFileDialog dialog( this, writing ? tr( L"Enregistrer sous", L"Save As" ) : tr( L"Charger un profil", L"Load Profile" ),
                                 _configuration.wstring(), writing ? L"knight-mod-profile.ini" : L"",
                                 tr( L"Profil Knight Mod (*.ini)|*.ini", L"Knight Mod Profile (*.ini)|*.ini" ),
                                 writing ? wxFD_SAVE | wxFD_OVERWRITE_PROMPT : wxFD_OPEN | wxFD_FILE_MUST_EXIST );
            if ( dialog.ShowModal() != wxID_OK )
                return;
            const fs::path path( dialog.GetPath().ToStdWstring() );
            if ( !storage::isIniProfile( path ) ) {
                error( L"Choisir un profil au format .ini.", L"Choose a profile in .ini format." );
                return;
            }
            if ( writing ) {
                if ( storage::sameFile( path, _ini ) ) {
                    error( L"Choisir un autre nom que knight_mod.ini.", L"Choose a name other than knight_mod.ini." );
                    return;
                }
                storage::replaceText( path, km::updateIni( "# Knight Mod profile\r\n", _values ) );
                _status->SetLabel( tr( L"Profil enregistré.", L"Profile saved." ) );
                return;
            }
            try {
                if ( !km::load( storage::readText( path ), _values, true ) ) {
                    error( L"Le profil ne contient pas de réglages Knight Mod valides.", L"The profile does not contain valid Knight Mod settings." );
                    return;
                }
            }
            catch ( const storage::FileTooLarge & ) {
                error( L"Le profil dépasse la taille maximale de 1 Mio.", L"The profile is larger than 1 MiB." );
                return;
            }
            showValues();
            fitPage();
            _status->SetLabel( tr( L"Profil chargé. Enregistrer ou lancer le jeu.", L"Profile loaded. Save or play to apply." ) );
        }

        void openPath( const fs::path & path )
        {
            if ( !wxLaunchDefaultApplication( path.wstring() ) )
                error( L"Impossible d’ouvrir le fichier ou le dossier.", L"Could not open the file or folder." );
        }

        void openDocument( const wchar_t * fr, const wchar_t * en )
        {
            auto directory = _mod / L"docs";
            if ( !storage::isDirectory( directory ) )
                directory = _installation / L"docs" / L"Knight Mod";
            const auto localized = directory / tr( fr, en );
            if ( storage::isFile( localized ) )
                openPath( localized );
            else if ( _french && storage::isFile( directory / en ) )
                openPath( directory / en );
            else
                error( L"Document introuvable dans Knight Mod\\docs.", L"Document not found in Knight Mod\\docs." );
        }

        void changeLanguage()
        {
            if ( !capture() )
                return;
            wxMenu languages;
            languages.AppendRadioItem( 400, L"English" );
            languages.AppendRadioItem( 401, L"Français" );
            languages.Check( _french ? 401 : 400, true );
            const int selection = GetPopupMenuSelectionFromUser( languages );
            if ( selection == wxID_NONE )
                return;
            const bool french = selection == 401;
            storage::ensureDirectory( _configuration );
            storage::replaceText( _languageFile, french ? "fr\n" : "en\n" );
            _french = french;
            localize();
            fitPage();
        }

        const fs::path _installation;
        const fs::path _mod;
        const fs::path _configuration;
        const fs::path _ini;
        const fs::path _languageFile;
        km::Values _values = km::defaults();
        km::Values _saved{};
        bool _french = false;
        bool _synchronizing = false;
        std::array<SettingRow, km::count> _rows{};
        std::array<LauncherButton *, 3> _tabs{};
        std::array<wxScrolledWindow *, 3> _pages{};
        wxSimplebook * _book{};
        wxCollapsiblePane * _conversion{}, * _requirements{};
        wxPanel * _example{};
        wxStaticText * _training{}, * _convert{}, * _restore{}, * _appearance{}, * _profiles{}, * _exampleLabel{}, * _quote{}, * _status{};
        wxButton * _saveProfile{}, * _loadProfile{}, * _defaults{}, * _guide{}, * _folder{}, * _save{}, * _play{}, * _quit{};
        wxSpinCtrl * _quantity{};
        wxTimer _helpTimer;
        wxTipWindow * _tip{};
        wxWindow * _helpAnchor{};
        size_t _helpSetting = 0;
    };

    class SettingsApp final : public wxApp
    {
        bool OnInit() override
        {
            wxString language;
            for ( int i = 1; i < argc; ++i ) {
                const wxString argument( argv[i] );
                if ( argument == L"--language=fr" || argument == L"--language=en" )
                    language = argument.AfterFirst( L'=' );
            }
            try {
                auto * frame = new SettingsFrame( language );
                SetTopWindow( frame );
                frame->Show();
                return true;
            }
            catch ( const std::exception & ) {
                wxMessageBox( L"Could not start Knight Mod Launcher. / Impossible d’ouvrir le lanceur.", L"Knight Mod Launcher", wxOK | wxICON_ERROR );
                return false;
            }
        }
    };
}

wxIMPLEMENT_APP( SettingsApp );
