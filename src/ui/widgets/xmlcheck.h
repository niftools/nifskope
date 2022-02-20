#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H


#include <QThread> // Inherited
#include <QWidget> // Inherited
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QWaitCondition>

#include <map>
#include <array>


class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QComboBox;
class QTextBrowser;

class TestMessage;
class FileSelector;


enum OpType
{
	OP_EQ,
	OP_NEQ,
	OP_AND,
	OP_AND_S,
	OP_NAND,
	OP_STR_S,
	OP_STR_E,
	OP_STR_NS,
	OP_STR_NE,
	OP_CONT

};

static std::array<QString, 10> ops_ord = { 
	// EQ, NEQ, AND, AND_S, NAND, STR_E, STR_S, STR_NS, STR_NE, CONT
	"==", "!=", "&", "& 1<<", "!&", "^", "$", "!^", "!$", QChar(0x2282) // ⊂
};

static std::map<QString, QPair<OpType, QString>> ops = {
	{ ops_ord[OP_EQ], {OP_EQ, "Equality"} },
	{ ops_ord[OP_NEQ], {OP_NEQ, "Inequality"} },
	{ ops_ord[OP_AND], {OP_AND, "Bitwise AND"} },
	{ ops_ord[OP_AND_S], {OP_AND_S, "Bitwise AND (Shifted)"} },
	{ ops_ord[OP_NAND], {OP_NAND, "Bitwise NAND"} },
	{ ops_ord[OP_STR_S], {OP_STR_S, "Starts With"} },
	{ ops_ord[OP_STR_E], {OP_STR_E, "Ends With"} },
	{ ops_ord[OP_STR_NS], {OP_STR_NS, "Does not start with"} },
	{ ops_ord[OP_STR_NE], {OP_STR_NE, "Does not end with"} },
	{ ops_ord[OP_CONT], {OP_CONT, "Contains"} }
};

class FileQueue final
{
public:
	FileQueue() {}

	QString dequeue();

	bool isEmpty() { return count() == 0; }
	int count();

	void init( const QString & directory, const QStringList & extensions, bool recursive );
	void clear();

protected:
	QQueue<QString> make( const QString & directory, const QStringList & extensions, bool recursive );

	QMutex mutex;
	QQueue<QString> queue;
};

class TestThread final : public QThread
{
	Q_OBJECT

public:
	TestThread( QObject * o, FileQueue * q );
	~TestThread();

	QString blockMatch;
	QString valueName;
	QString valueMatch;
	OpType op;
	quint32 verMatch = 0;
	bool reportAll = true;
	bool headerOnly = false;
	bool checkFile = true;

signals:
	void sigStart( const QString & file );
	void sigReady( const QString & result );

protected:
	void run() override final;

	QList<TestMessage> checkLinks( const class NifModel * nif, const class QModelIndex & iParent, bool kf );

	FileQueue * queue;

	QMutex quit;
};

//! The XML checker widget.
class TestShredder final : public QWidget
{
	Q_OBJECT

public:
	TestShredder();
	~TestShredder();

	static TestShredder * create();

protected slots:
	void chooseBlock();
	void run();
	void xml();

	void threadStarted();
	void threadFinished();

	void renumberThreads( int );

protected:
	void closeEvent( QCloseEvent * ) override final;

	FileSelector * directory;
	QLineEdit * blockMatch;
	QLineEdit * valueName;
	QLineEdit * valueMatch;
	QComboBox * valueOps;
	QCheckBox * recursive;
	QCheckBox * chkNif, * chkKf, * chkKfm, *chkCheckErrors;
	QCheckBox * repErr, * hdrOnly;
	QSpinBox * count;
	QLineEdit * verMatch;
	QTextBrowser * text;
	QProgressBar * progress;
	QLabel * label;
	QPushButton * btRun;

	FileQueue queue;

	QList<TestThread *> threads;

	QDateTime time;
};

#endif
