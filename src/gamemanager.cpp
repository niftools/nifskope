#include "gamemanager.h"

#include "bsa.h"
#include "message.h"

#include <QSettings>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QDir>

namespace Game
{

static const auto GAME_PATHS = QString("Game Paths");
static const auto GAME_FOLDERS = QString("Game Folders");
static const auto GAME_ARCHIVES = QString("Game Archives");
static const auto GAME_STATUS = QString("Game Status");
static const auto GAME_MGR_VER = QString("Game Manager Version");

static const QStringList ARCHIVE_EXT{"*.bsa", "*.ba2"};

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
	for ( const auto& finfo : path_dir.entryInfoList(ARCHIVE_EXT, QDir::Files) )
		list.append(finfo.absoluteFilePath());

	if ( folder.isEmpty() )
		return list;

	// Remove the archives that do not contain this folder
	QStringList list_filtered;
	for ( const auto& f : list )
		if ( GameManager::archive_contains_folder(f, folder) )
			list_filtered.append( f );
	return list_filtered;
}

QStringList existing_folders(GameMode game, QString path)
{
	// TODO: Restore the minimal previous support for detecting Civ IV, etc.
	if ( game == OTHER )
		return {};

	QStringList folders;
	for ( const auto& f : FOLDERS.value(game, {}) ) {
		QDir dir(QString("%1/%2").arg(path).arg(DATA.value(game, "")));
		if ( dir.exists(f) )
			folders.append(QFileInfo(dir, f).absoluteFilePath());
	}

	return folders;
}

GameManager::GameInfo get_game_info(GameMode game)
{
	GameManager::GameInfo info;
	info.id = game;
	info.name = StringForMode(game);
	info.path = registry_game_path(KEY.value(game, {}));
	return info;
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

QProgressDialog* prog_dialog( QString title )
{
	QProgressDialog* dlg = new QProgressDialog(title, {}, 0, NUM_GAMES);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->show();
	return dlg;
}

void process( QProgressDialog* dlg, int i )
{
	if ( dlg ) {
		dlg->setValue(i);
		QCoreApplication::processEvents();
	}
}

GameManager::GameManager()
{
	QSettings settings;
	int manager_version = settings.value(GAME_MGR_VER, 0).toInt();
	if ( manager_version == 0 ) {
		auto dlg = prog_dialog("Initializing the Game Manager");
		// Initial game manager settings
		init_settings(manager_version, dlg);
		dlg->close();
	}

	if ( manager_version == 1 ) {
		update_settings(manager_version);
	}

	load();
	load_archives();
}

GameMode GameManager::get_game( uint32_t version, uint32_t user, uint32_t bsver )
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
		if ( user == 10 || version == 0x14000005 ) // TODO: Enumeration
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

	// NOTE: Morrowind shares a version with other games (Freedom Force, etc.)
	if ( version == 0x04000002 )
		return MORROWIND;

	return OTHER;
}

GameManager* GameManager::get()
{
	static auto instance{new GameManager{}};
	return instance;
}

void GameManager::init_settings( int& manager_version, QProgressDialog* dlg ) const
{
	QSettings settings;
	QVariantMap paths, folders, archives, status;
	for ( int g = 0; g < NUM_GAMES; g++ ) {
		process(dlg, g);
		auto game = get_game_info(GameMode(g));
		if ( game.path.isEmpty() )
			continue;
		paths.insert(game.name, game.path);
		folders.insert(game.name, existing_folders(game.id, game.path));

		// Filter and insert archives
		QStringList filtered;
		if ( game.id == FALLOUT_4 || game.id == FALLOUT_76 )
			filtered.append(archives_list(game.path, DATA.value(game.id, {}), "materials"));
		filtered.append(archives_list(game.path, DATA.value(game.id, {}), "textures"));
		filtered.removeDuplicates();

		archives.insert(game.name, filtered);

		// Game Enabled Status
		status.insert(game.name, !game.path.isEmpty());
	}

	settings.setValue(GAME_PATHS, paths);
	settings.setValue(GAME_FOLDERS, folders);
	settings.setValue(GAME_ARCHIVES, archives);
	settings.setValue(GAME_STATUS, status);
	settings.setValue(GAME_MGR_VER, ++manager_version);
}

void GameManager::update_settings( int& manager_version, QProgressDialog * dlg ) const
{
	QSettings settings;
	QVariantMap folders;

	for ( int g = 0; g < NUM_GAMES; g++ ) {
		process(dlg, g);
		auto game = get_game_info(GameMode(g));
		if ( game.path.isEmpty() )
			continue;

		if ( manager_version == 1 )
			folders.insert(game.name, existing_folders(game.id, game.path));
	}

	if ( manager_version == 1 ) {
		settings.setValue(GAME_FOLDERS, folders);
		manager_version++;
	}

	settings.setValue(GAME_MGR_VER, manager_version);
}

QList<FSArchiveFile*> GameManager::opened_archives( const GameMode game )
{
	if ( !status(game) )
		return {};

	QList<FSArchiveFile *> archives;
	for ( const auto an : get()->handles.value(game) ) {
		archives.append(an->getArchive());
	}
	return archives;
}

bool GameManager::archive_contains_folder( const QString& archive, const QString& folder )
{
	if ( BSA::canOpen(archive) ) {
		auto bsa = std::make_unique<BSA>(archive);
		if ( bsa && bsa->open() )
			return bsa->getRootFolder()->children.contains(folder.toLower());
	}
	return false;
}

QString GameManager::path( const GameMode game )
{
	return get()->game_paths.value(game, {});
}

QString GameManager::data( const GameMode game )
{
	return path(game) + "/" + DATA[game];
}

QStringList GameManager::folders( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return folders(FALLOUT_NV) + folders(FALLOUT_3);
	if ( status(game) )
		return get()->game_folders.value(game, {});
	return {};
}

QStringList GameManager::archives( const GameMode game )
{
	if ( game == FALLOUT_3NV )
		return archives(FALLOUT_NV) + archives(FALLOUT_3);
	if ( status(game) )
		return get()->game_archives.value(game, {});
	return {};
}

bool GameManager::status(const GameMode game)
{
	if ( game == FALLOUT_3NV )
		return status(FALLOUT_3) || status(FALLOUT_NV);
	return get()->game_status.value(game, false);
}

QStringList GameManager::find_folders(const GameMode game)
{
	return existing_folders(game, get()->game_paths.value(game, {}));
}

QStringList GameManager::find_archives( const GameMode game )
{
	QDir data_dir = QDir(GameManager::data(game));
	if ( !data_dir.exists() )
		return {};

	QStringList archive_paths;
	for ( const auto& a : data_dir.entryList(ARCHIVE_EXT, QDir::Files) )
		archive_paths.append(data_dir.absoluteFilePath(a));

	return archive_paths;
}

QStringList GameManager::filter_archives( const QStringList& list, const QString& folder )
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

void GameManager::save() const
{
	QSettings settings;
	QVariantMap paths, folders, archives, status;
	for ( const auto& p : game_paths.toStdMap() )
		paths.insert(StringForMode(p.first), p.second);
	
	for ( const auto& f : game_folders.toStdMap() )
		folders.insert(StringForMode(f.first), f.second);

	for ( const auto& a : game_archives.toStdMap() )
		archives.insert(StringForMode(a.first), a.second);

	for ( const auto& s : game_status.toStdMap() )
		status.insert(StringForMode(s.first), s.second);

	settings.setValue(GAME_PATHS, paths);
	settings.setValue(GAME_FOLDERS, folders);
	settings.setValue(GAME_ARCHIVES, archives);
	settings.setValue(GAME_STATUS, status);
}

void GameManager::load()
{
	QMutexLocker locker(&mutex);
	QSettings settings;
	for ( const auto& p : settings.value(GAME_PATHS).toMap().toStdMap() )
		game_paths[ModeForString(p.first)] = p.second.toString();

	for ( const auto& f : settings.value(GAME_FOLDERS).toMap().toStdMap() )
		game_folders[ModeForString(f.first)] = f.second.toStringList();

	for ( const auto& a : settings.value(GAME_ARCHIVES).toMap().toStdMap() )
		game_archives[ModeForString(a.first)] = a.second.toStringList();

	for ( const auto& s : settings.value(GAME_STATUS).toMap().toStdMap() )
		game_status[ModeForString(s.first)] = s.second.toBool();
}

void GameManager::load_archives()
{
	QMutexLocker locker(&mutex);
	// Reset the currently open archive handles
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

void GameManager::clear()
{
	QMutexLocker locker(&mutex);
	game_paths.clear();
	game_folders.clear();
	game_archives.clear();
	game_status.clear();
}

void GameManager::insert_game( const GameMode game, const QString& path )
{
	QMutexLocker locker(&mutex);
	game_paths.insert(game, path);
}

void GameManager::insert_folders( const GameMode game, const QStringList& list )
{
	QMutexLocker locker(&mutex);
	game_folders.insert(game, list);
}

void GameManager::insert_archives( const GameMode game, const QStringList& list )
{
	QMutexLocker locker(&mutex);
	game_archives.insert(game, list);
}

void GameManager::insert_status( const GameMode game, bool status )
{
	QMutexLocker locker(&mutex);
	game_status.insert(game, status);
}

} // end namespace Game
