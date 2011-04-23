#include "3dview_OGRE.h"
#define OVERRIDE

NifSkopeOgre3D *NifSkopeOgre3D::create()
{
	NifSkopeOgre3D *tmp = new NifSkopeOgre3D();
	/*try {
            tmp->go();
	} catch( Ogre::Exception& e ) {
		//#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		//MessageBox( NULL, e.getFullDescription().c_str(), "An exception hasoccured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		//#else
		std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
		//#endif
	}*/
	return tmp;
}

NifSkopeOgre3D::NifSkopeOgre3D(void)
	: NifSkopeQt3D (NULL)
	,ready(0), mRoot(0), mCam(0), mScn(0), mWin(0)
{
}

NifSkopeOgre3D::~NifSkopeOgre3D(void)
{
	//delete mRoot;
}

bool NifSkopeOgre3D::go()
{
	mRoot = new Ogre::Root("", "", "NifSkopeOgre3D.log");
	// A list of required plugins
	Ogre::StringVector required_plugins;
	required_plugins.push_back("GL RenderSystem");
	required_plugins.push_back("Octree & Terrain Scene Manager");
	// List of plugins to load
	Ogre::StringVector plugins_toLoad;
	plugins_toLoad.push_back("RenderSystem_GL");
	plugins_toLoad.push_back("RenderSystem_Direct3D9");
	plugins_toLoad.push_back("Plugin_OctreeSceneManager");
	// Load the OpenGL RenderSystem and the Octree SceneManager plugins
	for (Ogre::StringVector::iterator j = plugins_toLoad.begin(); j != plugins_toLoad.end(); j++)
	{
		#ifdef _DEBUG
		mRoot->loadPlugin(*j + Ogre::String("_d"));
		#else
		mRoot->loadPlugin(*j);
		#endif;
	}
	// Check if the required plugins are installed and ready for use
	// If not: exit the application
	Ogre::Root::PluginInstanceList ip = mRoot->getInstalledPlugins();
	for (Ogre::StringVector::iterator j = required_plugins.begin(); j != required_plugins.end(); j++) {
		bool found = false;
		// try to find the required plugin in the current installed plugins
		for (Ogre::Root::PluginInstanceList::iterator k = ip.begin(); k != ip.end(); k++) {
			if ((*k)->getName() == *j) {
				found = true;
				break;
			}
		}
		if (!found) {  // return false because a required plugin is not available
			return false;
		}
	}
	//-------------------------------------------------------------------------------------
	// setup resources
	// Only add the minimally required resource locations to load up the Ogre head mesh
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation("D:/projects/nifskope/debug/media/materials/programs", "FileSystem", "General");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation("D:/projects/nifskope/debug/media/materials/scripts", "FileSystem", "General");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation("D:/projects/nifskope/debug/media/materials/textures", "FileSystem", "General");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation("D:/projects/nifskope/debug/media/models", "FileSystem", "General");		
	//-------------------------------------------------------------------------------------
	// configure
	// Grab the OpenGL RenderSystem, or exit
	Ogre::RenderSystem* rs = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
	//Ogre::RenderSystem* rs = mRoot->getRenderSystemByName("Direct3D9 Rendering Subsystem");
	if(!(rs->getName() == "OpenGL Rendering Subsystem")) {
	//if(!(rs->getName() == "Direct3D9 Rendering Subsystem")) {
		return false; //No RenderSystem found
	}
	// configure our RenderSystem
	rs->setConfigOption("Full Screen", "No");
	rs->setConfigOption("VSync", "Yes");
	rs->setConfigOption("Video Mode", "800 x 600 @ 32-bit");
	mRoot->setRenderSystem(rs);
	Ogre::NameValuePairList misc;
	//misc["parentWindowHandle"] = Ogre::StringConverter::toString((long)window_provider.winId());
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((long)winId());
	misc["vsync"] = "true";// it ignores the above when "createRenderWindow"
	//mWin = mRoot->createRenderWindow("mu", 800, 600, false);
	//  ogre created window 
	mWin = mRoot->initialise(false, "mu");
	mWin = mRoot->createRenderWindow("mu", width(), height(), false, &misc);
	//-------------------------------------------------------------------------------------
	// choose scenemanager
	// Get the SceneManager, in this case the OctreeSceneManager
	mScn = mRoot->createSceneManager("OctreeSceneManager", "DefaultSceneManager");
	//-------------------------------------------------------------------------------------
	// create camera
	// Create the camera
	mCam = mScn->createCamera("PlayerCam");
	// Position it at 500 in Z direction
	mCam->setPosition(Ogre::Vector3(0,0,80));
	// Look back along -Z
	mCam->lookAt(Ogre::Vector3(0,0,-300));
	mCam->setNearClipDistance(5);
	//-------------------------------------------------------------------------------------
	// create viewports
	// Create one viewport, entire window
	Ogre::Viewport* vp = mWin->addViewport(mCam);
	vp->setBackgroundColour(Ogre::ColourValue(0,0,0));
	// Alter the camera aspect ratio to match the viewport
	mCam->setAspectRatio(
		Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
	//-------------------------------------------------------------------------------------
	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	//-------------------------------------------------------------------------------------
	// Create any resource listeners (for loading screens)
	//createResourceListener();
	//-------------------------------------------------------------------------------------
	// load resources
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	//-------------------------------------------------------------------------------------
	// Create the scene
	Ogre::Entity* ogreHead = mScn->createEntity("Head", "ogrehead.mesh");
	Ogre::SceneNode* headNode = mScn->getRootSceneNode()->createChildSceneNode();
	headNode->attachObject(ogreHead);
	// Set ambient light
	mScn->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
	// Create a light
	Ogre::Light* l = mScn->createLight("MainLight");
	l->setPosition(20,80,50);
	ready = 1;

	/*
	//-------------------------------------------------------------------------------------
	//mRoot->startRendering();
	while(true) {
		// Pump window messages for nice behaviour
		Ogre::WindowEventUtilities::messagePump();
		// Render a frame
		mRoot->renderOneFrame();
		if(mWin->isClosed())	{
			return false;
		}
	}
	// We should never be able to reach this corner
	// but return true to calm down our compiler
	return true;
	*/
	return ready;
}// go

OVERRIDE void NifSkopeOgre3D::resizeEvent(QResizeEvent *p)// overrides
{
	if (!ready)
		return;
	//mWin->resize(width(), height());
	mWin->windowMovedOrResized();
	//mCam->setAspectRatio(1);
}

OVERRIDE void NifSkopeOgre3D::paintEvent(QPaintEvent *p)
{
	if (!ready)
		return;
	// Pump window messages for nice behaviour
	Ogre::WindowEventUtilities::messagePump();
	// Render a frame
	mRoot->renderOneFrame();
	//if(mWin->isClosed())	{
	//	return false;
	//}
}
