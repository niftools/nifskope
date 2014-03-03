// Please run doxygen and view apidocs/html/index.html

/*!
 * \mainpage NifSkope API Documentation
 *
 * A concise description of nifskope's inner workings should come here, with
 * pointers to get people started.
 *
 * The <a href="annotated.html">class list</a> and
 * <a href="files.html">file list</a> will eventually contain brief summaries
 * of what every class is used for, and what every file contains, respectively.
 *
 * - \ref intro
 * - \ref parsing
 *
 * \page intro Introduction
 *
 * %NifSkope is a graphical program that allows you to open NIF files, view
 * their contents, edit them, and write them back out again. It is written in
 * C++ using OpenGL and the Qt framework, and designed using the
 * <a href="http://doc.trolltech.com/latest/model-view-programming.html">Model/View
 * Programming</a> paradigm.
 *
 * The main application is present in the NifSkope class; rendering takes place
 * via GLView.  A central feature of Qt is the
 * <a href="http://doc.trolltech.com/latest/signalsandslots.html">Signals and Slots</a>
 * mechanism, which is used extensively to build the GUI.  A NIF is internally represented
 * as a NifModel, and blocks are referenced by means of QModelIndex and
 * QPersistentModelIndex.
 *
 * Various "magic" functions can be performed on a NIF via the Spell system;
 * this is probably a good place to start if you want to learn about how a NIF
 * is typically structured and how the blocks are manipulated.
 *
 * <center>Next: \ref parsing</center>
 *
 * \page parsing Reading and parsing a NIF
 *
 * The NIF specification is currently described by nif.xml and parsed using NifXmlHandler.
 *
 */

