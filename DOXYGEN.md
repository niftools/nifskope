Main Page {#mainpage}
=========

[TOC]


Introduction {#intro}
========

%NifSkope is a graphical program that allows you to open NIF files, view their contents, edit them, and write them back out again. It is written in C++ using OpenGL and the Qt framework.

The main application resides in the NifSkope class; rendering takes place via GLView.  A central feature of Qt is the [Signals and Slots](http://doc.qt.io/qt-5/signalsandslots.html) mechanism, which is used extensively for communication between classes.  A NIF is internally represented as a NifModel, and blocks are referenced by means of QModelIndex or QPersistentModelIndex.

Various functions can be performed on a NIF via the Spell system; this is a good place to start if you want to learn about how a NIF is typically structured and how the blocks are manipulated.


Reading and parsing a NIF {#parsing}
========

The NIF specification is currently described by [nif.xml](https://github.com/niftools/nifxml) and parsed using NifXmlHandler.


Detailed Information {#detailed}
========

- @ref ui_programming 
- @ref viewport_details

[Signals and Slots]: 	http://doc.qt.io/qt-5/signalsandslots.html

<!-- ~~~~~~~ end mainpage ~~~~~~~ -->


@page ui_programming UI Programming

[TOC]

%NifSkope uses [Qt Designer] and [.ui files] to define the layout of most of its UI and uses [Signals and Slots] to allow communication between UI and non-UI code.  %NifSkope makes partial use of QMetaObject's [auto-connection] features.

The UI is styled with a Qt-specific subset of CSS dubbed [QSS]. Most of the style is defined in `style.qss` which is installed alongside NifSkope.exe, though stylesheets can also be defined in C++ using QWidget::setStyleSheet().


See also: [Designing a UI].

[Designing a UI]: 		http://doc.qt.io/qt-5/gettingstartedqt.html#designing-a-ui
[Qt Designer]: 			http://doc.qt.io/qt-5/qtdesigner-manual.html
[.ui files]: 			http://doc.qt.io/qt-5/designer-using-a-ui-file.html
[Signals and Slots]: 	http://doc.qt.io/qt-5/signalsandslots.html
[auto-connection]: 		http://doc.qt.io/qt-5/designer-using-a-ui-file.html#widgets-and-dialogs-with-auto-connect
[QSS]: 					http://doc.qt.io/qt-5/stylesheet-reference.html

<!-- ~~~~~~~ end ui_programming ~~~~~~~ -->


@page viewport_details Viewport

[TOC]

The main viewport class is GLView. Each frame is painted to the viewport via GLView::paintGL(). The viewport scenegraph is managed by a Scene class. A scene consists of:

- Node, a physical object in the scene such as NiNode, BSFadeNode, etc.
    - Mesh, e.g. NiTriShape
	- Particles
	- LODNode, BillboardNode
- Property, a property of a physical object in the scene.

Nodes {#nodes}
========

The Node class is the base class for any physical object in a Scene.  A Node can have children which are stored in a NodeList. [Properties] are stored with the Node via a PropertyList.


Properties {#properties}
========

The Property class is the %NifSkope analog to NiProperty blocks.  Ideally anything that inherits NiProperty in nif.xml should have a Property subclass implementation.  Not all Properties require any kind of manifestation in the scene but they are nevertheless encapsulated and tracked by a Property class.


Animation {#animation}
========

Anything that should animate must implement the IControllable interface.  To animate an IControllable, you create a Controller.  These Controllers are then made a `friend class` in the class which implements IControllable.  There are currently very few Controllers implemented when it comes to reaching feature parity with the NIF specification:

| %NifSkope class                | NIF Block                        |
|--------------------------------|----------------------------------|
| KeyframeController             | NiKeyframeController             |
| TransformController            | NiTransformController            |
| MultiTargetTransformController | NiMultiTargetTransformController |
| VisibilityController           | NiVisController                  |
| MorphController                | NiGeomMorpherController          |
| UVController                   | NiUVController                   |
| ParticleController             | NiParticleSystemController       |
| AlphaController                | NiAlphaController                |
| MaterialColorController        | NiMaterialColorController        |
| TexFlipController              | NiFlipController                 |
| TexTransController             | NiTextureTransformController     |

Newer Bethesda NIFs mostly use Bethesda proprietary (BS*) blocks, none of which are currently supported.  This includes `BSKeyframeController, BSEffectShaderPropertyFloatController, BSEffectShaderPropertyColorController, BSLightingShaderPropertyFloatController, BSLightingShaderPropertyColorController` to name a few.

To utilize [Controllers] a class must implement IControllable's methods:

- IControllable::clear()
- IControllable::update()
- IControllable::transform()
- IControllable::setController()

[Properties]: @ref Property
[Controllers]: @ref Controller

<!-- ~~~~~~~ end viewport_details ~~~~~~~ -->
