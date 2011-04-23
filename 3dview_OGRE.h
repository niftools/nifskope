#ifndef __3dview_OGRE_H__
#define __3dview_OGRE_H__

#include "3dview.h"

// 2.add OGRE
#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>

#include <OgreLogManager.h>
#include <OgreViewport.h>
#include <OgreEntity.h>
#include <OgreWindowEventUtilities.h>
#include <OgrePlugin.h>

class NifSkopeOgre3D: public NifSkopeQt3D
{
private:
	int ready;
public:
	static NifSkopeOgre3D * create();
	NifSkopeOgre3D(void);
	virtual ~NifSkopeOgre3D(void);
	bool go();
	void resizeEvent(QResizeEvent *p);
	void paintEvent(QPaintEvent *p);
protected:
	Ogre::Root *mRoot;
	Ogre::Camera *mCam;
	Ogre::SceneManager *mScn;
	Ogre::RenderWindow *mWin;
};

#endif /* __3dview_OGRE_H__ */
