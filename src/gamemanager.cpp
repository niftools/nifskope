#include "gamemanager.h"

#include "bsa.h"
#include "message.h"

#include <QSettings>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QDir>

namespace Game
{

QString registry_game_path( const QString& key )
{
#ifdef Q_OS_WIN32
	QString data_path;
	QSettings cfg(key, QSettings::Registry32Format);
	data_path = cfg.value("Installed Path").toString(); // Steam
	if ( data_path.isEmpty() )
		data_path = cfg.value("Path").toString(); // Microsoft Uninstall
	// Remove encasing quotes
	data_path.remove('"');
	if ( data_path.isEmpty() )
		return {};

	QDir data_path_dir(data_path);
	if ( data_path_dir.exists() )
		return QDir::cleanPath(data_path);

#endif
	return {};
}

QStringList archives_list( const QString& path, const QString& data_dir, const QString& folder = {} )
{
	if ( path.isEmpty() )
		return {};

	QStringList list;
	QDir path_dir(path);
	if ( path_dir.exists(data_dir) )
		path_dir.cd(data_dir);
	for ( const auto& finfo : path_dir.entryInfoList({"*.bsa", "*.ba2"}, QDir::Files) )
		list << finfo.absoluteFilePath();

	if ( folder.isEmpty() )
		return list;

	// Remove the archives that do not contain this folder
	QStringList list_filtered;
	for ( const auto& f : list )
		if ( GameManager::archive_contains_folder(f, folder) )
			list_filtered.append( f );
	return list_filtered;
}

QString StringForMode( GameMode game )
{
	if ( game >= NUM_GAMES )
		return {};

	return STRING.value(game, "");
}

GameMode ModeForString( QString game )
{
	return STRING.key(game, OTHER);
}

GameManager::GameManager()
{
	QSettings settings;
	int manager_version = settings.value("Game Manager Version", 0).toInt();
	if ( manager_version == 0 ) {
		manager_version++;
		QProgressDialog* dlg = new QProgressDialog( "Initializing the Game Manager", {}, 0, NUM_GAMES );
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->show();
		// Initial game manager settings
		initialize(manager_version, dlg);
		dlg->close();
	}

	load();
	reset_archive_handles();
}

GameMode GameManager::GetGame( uint32_t version, uint32_t user, uint32_t bsver )
{
	switch ( bsver ) {
	case 0:
		break;
	case BSSTREAM_1:
	case BSSTREAM_3:
	case BSSTREAM_4:
	case BSSTREAM_5:
	case BSSTREAM_6:
	case BSSTREAM_7:
	case BSSTREAM_8:
	case BSSTREAM_9:
		return OBLIVION;
	case BSSTREAM_11:
		if ( user == 10 ) // TODO: Enumeration
			return OBLIVION;
		else if ( user == 11 )
			return FALLOUT_3NV;
		return OTHER;
	case BSSTREAM_14:
	case BSSTREAM_16:
	case BSSTREAM_21:
	case BSSTREAM_24:
	case BSSTREAM_25:
	case BSSTREAM_26:
	case BSSTREAM_27:
	case BSSTREAM_28:
	case BSSTREAM_30:
	case BSSTREAM_31:
	case BSSTREAM_32:
	case BSSTREAM_33:
	case BSSTREAM_34:
		return FALLOUT_3NV;
	case BSSTREAM_83:
		return SKYRIM;
	case BSSTREAM_100:
		return SKYRIM_SE;
	case BSSTREAM_130:
		return FALLOUT_4;
	case BSSTREAM_155:
		return FALLOUT_76;
	default:
		break;
	};

	return OTHER;
}

static GameManager * g_game_manager = nullptr;

GameManager * GameManager::get()
{
	if ( !g_game_manager )
		g_game_manager = new GameManager();
	return g_game_manager;
}

void GameManager::del()
{
	if ( g_game_manager ) {
		delete g_game_manager;
		g_game_manager = nullptr;
	}
}

void GameManager::initialize( int manager_version, QProgressDialog* dlg )
{
	QSettings settings;
	QMap<QString, QVariant> paths;
	QMap<QString, QVariant> folders;
	QMap<QString, QVariant> archives;
	QMap<QString, QVariant> status;
	for ( const auto & key : KEY.toStdMap() ) {
		// Progress Bar
		if ( dlg ) {
			dlg->setValue(key.first);
			QCoreApplication::processEvents();
		}
		if ( key.second.isEmpty() )
			continue;
		auto game = key.first;
		auto game_str = StringForMode(game);
		auto path = registry_game_path(key.second);
		paths.insert(game_str, path);
		folders.insert(game_str, FOLDERS.value(game, {}));

		// Filter and insert archives
		QStringList filtered;
		if ( game == FALLOUT_4 || game == FALLOUT_76 )
			filtered.append(archives_list(path, DATA.value(game, {}), "materials"));
		filtered.append(archives_list(path, DATA.value(game, {}), "textures"));
		filtered.removeDuplicates();

		archives.insert(game_str, filtered);

		// Game Enabled Status
		status.insert(game_str, !path.isEmpty());
	}

	settings.setValue("Game Paths", paths);
	settings.setValue("Game Folders", folders);
	settings.setValue("Game Archives", archives);
	settings.setValue("Game Status", status);
	settings.setValue("Game Manager Version", manager_version);
}

QList<FSArchiveFile*> GameManager::get_archive_handles( Game::GameMode game )
{
	if ( get()->get_game_status(game) == false )
		return {};

	QList<FSArchiveFile *> archives;
	for ( std::shared_ptr<FSArchiveHandler> an : get()->handles.value(game) ) {
		archives.append(an->getArchive());
	}
	return archives;
}

bool GameManager::archive_contains_folder( const QString& archive, const QString& folder )
{
	bool contains = false;
	if ( BSA::canOpen(archive) ) {
		BSA * bsa = new BSA(archive);
		if ( bsa && bsa->open() ) {
			contains = bsa->getRootFolder()->children.contains(folder.toLower());
		}
		delete bsa;
	}
	return contains;
}

void GameManager::reset_archive_handles()
{
	handles.clear();
	for ( const auto ar : game_archives.toStdMap() ) {
		for ( const auto an : ar.second ) {
			// Skip loading of archives for disabled games
			if ( game_status.value(ar.first, false) == false )
				continue;
			if ( auto a = FSArchiveHandler::openArchive(an) )
				handles[ar.first].append(a);
		}
	}
}

QString GameManager::get_game_path( const GameMode game )
{
	return get()->game_paths.value(game, {});
}

QString GameManager::get_game_path( const QString& game )
{
	return get()->game_paths.value(ModeForString(game), {});
}

QString GameManager::get_data_path( const GameMode game )
{
	return get_game_path(game) + "/" + DATA[game];
}

QString GameManager::get_data_path( const QString& game )
{
	return get_data_path(ModeForString(game));
}

QStringList GameManager::get_folder_list( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return get_folder_list(FALLOUT_NV) + get_folder_list(FALLOUT_3);
	if ( get_game_status(game) )
		return get()->game_folders.value(game, {});
	return {};
}
QStringList GameManager::get_folder_list( const QString& game )
{
	return get_folder_list(ModeForString(game));
}

QStringList GameManager::get_archive_list( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return get_archive_list(FALLOUT_NV) + get_archive_list(FALLOUT_3);
	if ( get_game_status(game) )
		return get()->game_archives.value(game, {});
	return {};
}


QStringList GameManager::get_archive_list( const QString& game )
{
	return get_archive_list(ModeForString(game));
}

QStringList GameManager::get_archive_file_list( const GameMode game )
{
	QDir data_dir = QDir(GameManager::get_data_path(game));
	if ( !data_dir.exists() )
		return {};

	QStringList data_archives = data_dir.entryList({"*.bsa", "*.ba2"}, QDir::Files);
	QStringList archive_paths;
	for ( const auto& a : data_archives ) {
		archive_paths << data_dir.absoluteFilePath(a);
	}

	return archive_paths;
}

QStringList GameManager::get_archive_file_list( const QString& game )
{
	return get_archive_file_list(ModeForString(game));
}

QStringList GameManager::get_filtered_archives_list( const QStringList& list, const QString& folder )
{
	if ( folder.isEmpty() )
		return list;

	QStringList filtered;
	for ( auto f : list ) {
		if ( archive_contains_folder(f, folder) )
			filtered.append(f);
	}
	return filtered;
}

void GameManager::set_game_status( const GameMode game, bool status )
{
	get()->game_status[game] = status;
}

void GameManager::set_game_status( const QString& game, bool status )
{
	set_game_status(ModeForString(game), status);
}

bool GameManager::get_game_status( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return get_game_status(FALLOUT_3) || get_game_status(FALLOUT_NV);
	return get()->game_status.value(game, false);
}

bool GameManager::get_game_status( const QString& game )
{
	return get_game_status(ModeForString(game));
}

void GameManager::update_game( const QString& game, const QString& path )
{
	get()->game_paths.insert(ModeForString(game), path);
}

void GameManager::update_folders( const QString& game, const QStringList& list )
{
	get()->game_folders.insert(ModeForString(game), list);
}

void GameManager::update_archives( const QString& game, const QStringList& list )
{
	get()->game_archives.insert(ModeForString(game), list);
}

void GameManager::save()
{
	QSettings settings;

	QMap<QString, QVariant> paths;
	QMap<QString, QVariant> folders;
	QMap<QString, QVariant> archives;
	QMap<QString, QVariant> status;
	
	for ( const auto& p : game_paths.toStdMap() )
		paths.insert(StringForMode(p.first), p.second);
	
	for ( const auto& f : game_folders.toStdMap() )
		folders.insert(StringForMode(f.first), f.second);

	for ( const auto& a : game_archives.toStdMap() )
		archives.insert(StringForMode(a.first), a.second);

	for ( const auto& s : game_status.toStdMap() )
		status.insert(StringForMode(s.first), s.second);

	settings.setValue("Game Paths", paths);
	settings.setValue("Game Folders", folders);
	settings.setValue("Game Archives", archives);
	settings.setValue("Game Status", status);
}

void GameManager::load()
{
	QSettings settings;

	game_paths.clear();
	auto paths = settings.value("Game Paths").toMap().toStdMap();
	for ( const auto& p : paths ) {
		game_paths[ModeForString(p.first)] = p.second.toString();
	}
	
	game_folders.clear();
	auto fol = settings.value("Game Folders").toMap().toStdMap();
	for ( const auto& f : fol ) {
		game_folders[ModeForString(f.first)] = f.second.toStringList();
	}

	game_archives.clear();
	auto arc = settings.value("Game Archives").toMap().toStdMap();
	for ( const auto& a : arc ) {
		game_archives[ModeForString(a.first)] = a.second.toStringList();
	}
	
	game_status.clear();
	auto stat = settings.value("Game Status").toMap().toStdMap();
	for ( const auto& s : stat ) {
		game_status[ModeForString(s.first)] = s.second.toBool();
	}
}

} // end namespace Game
