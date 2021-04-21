#include "spellbook.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>


// Brief description is deliberately not autolinked to class Spell
/*! \file flags.cpp
 * \brief Flag editing spells (spEditFlags)
 *
 * All classes here inherit from the Spell class.
 */

//! Edit flags
class spEditFlags : public Spell
{
public:
	QString name() const override { return Spell::tr( "Flags" ); }
	bool instant() const override { return true; }
	QIcon icon() const override { return QIcon( ":/img/flag" ); }

	//! Node / Property types on which flags are applicable
	enum FlagType
	{
		Alpha,
		Billboard,
		Controller,
		MatColControl,
		Node,
		RigidBody,
		Shape,
		Stencil,
		TexDesc,
		VertexColor,
		ZBuffer,
		BSX,
		NiAVObject,
		None
	};

	//! Find the index of flags relative to a given NIF index
	QModelIndex getFlagIndex( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->itemName( index ) == "Flags" && nif->isNiBlock( index.parent() ) )
			return index;

		if ( nif->isNiBlock( index ) )
			return nif->getIndex( index, "Flags" );

		if ( nif->inherits( nif->getBlock( index ), "bhkRigidBody" ) ) {
			QModelIndex iFlags = nif->getIndex( nif->getBlock( index ), "Col Filter" );
			iFlags = iFlags.sibling( iFlags.row(), NifModel::ValueCol );

			if ( index == iFlags )
				return iFlags;
		} else if ( nif->inherits( nif->getBlock( index ), "BSXFlags" ) ) {
			QModelIndex iFlags = nif->getIndex( nif->getBlock( index ), "Integer Data" );
			iFlags = iFlags.sibling( iFlags.row(), NifModel::ValueCol );

			if ( index == iFlags )
				return iFlags;
		} else if ( nif->itemName( index ) == "Flags" && nif->itemType( index.parent() ) == "TexDesc" ) {
			return index;
		}

		return QModelIndex();
	}

	//! Determine the applicable flag editing dialog for a NIF block type
	FlagType queryType( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->getValue( index ).isCount() ) {
			QString name = nif->itemName( index.parent() );

			if ( name == "NiAlphaProperty" ) {
				return Alpha;
			} else if ( name == "NiBillboardNode" ) {
				return Billboard;
			} else if ( nif->inherits( name, "NiTimeController" ) ) {
				if ( name == "NiMaterialColorController" ) {
					return MatColControl;
				}

				return Controller;
			} else if ( name == "NiNode" && nif->getVersionNumber() != 0x14020007 ) {
				return Node;
			} else if ( name == "bhkRigidBody" || name == "bhkRigidBodyT" ) {
				return RigidBody;
			} else if ( (name == "NiTriShape" || name == "NiTriStrips") && nif->getVersionNumber() != 0x14020007 ) {
				return Shape;
			} else if ( name == "NiStencilProperty" ) {
				return Stencil;
			} else if ( nif->itemType( index.parent() ) == "TexDesc" ) {
				return TexDesc;
			} else if ( name == "NiVertexColorProperty" ) {
				return VertexColor;
			} else if ( name == "NiZBufferProperty" ) {
				return ZBuffer;
			} else if ( name == "BSXFlags" ) {
				return BSX;
			} else if ( nif->inherits( index.parent(), "NiAVObject" ) && nif->getVersionNumber() == 0x14020007 ) {
				return NiAVObject;
			}
		}

		return None;
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override
	{
		return queryType( nif, getFlagIndex( nif, index ) ) != None;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override
	{
		QModelIndex iFlags = getFlagIndex( nif, index );

		switch ( queryType( nif, iFlags ) ) {
		case Alpha:
			alphaFlags( nif, iFlags );
			break;
		case Billboard:
			billboardFlags( nif, iFlags );
			break;
		case Controller:
			controllerFlags( nif, iFlags );
			break;
		case MatColControl:
			matColControllerFlags( nif, iFlags );
			break;
		case Node:
			nodeFlags( nif, iFlags );
			break;
		case RigidBody:
			bodyFlags( nif, iFlags );
			break;
		case Shape:
			shapeFlags( nif, iFlags );
			break;
		case Stencil:
			stencilFlags( nif, iFlags );
			break;
		case TexDesc:
			texDescFlags( nif, iFlags );
			break;
		case VertexColor:
			vertexColorFlags( nif, iFlags );
			break;
		case ZBuffer:
			zbufferFlags( nif, iFlags );
			break;
		case BSX:
			bsxFlags( nif, iFlags );
			break;
		case NiAVObject:
			niavFlags( nif, iFlags );
			break;
		default:
			break;
		}

		return index;
	}

	//! Set flags on an AlphaProperty
	void alphaFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		if ( !flags )
			flags = 0xed;

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		// See glBlendFunc; ONE and ZERO appear to swapped from where they should be (not to mention missing values before SATURATE)
		QStringList blendModes{
			Spell::tr( "One" ),
			Spell::tr( "Zero" ),
			Spell::tr( "Src Color" ),
			Spell::tr( "Inv Src Color" ),
			Spell::tr( "Dst Color" ),
			Spell::tr( "Inv Dst Color" ),
			Spell::tr( "Src Alpha" ),
			Spell::tr( "Inv Src Alpha" ),
			Spell::tr( "Dst Alpha" ),
			Spell::tr( "Inv Dst Alpha" ),
			Spell::tr( "Src Alpha Saturate" )
		};
		// ALWAYS and NEVER are swapped from where they should be; see glAlphaFunc
		QStringList testModes{
			Spell::tr( "Always" ),
			Spell::tr( "Less" ),
			Spell::tr( "Equal" ),
			Spell::tr( "Less or Equal" ),
			Spell::tr( "Greater" ),
			Spell::tr( "Not Equal" ),
			Spell::tr( "Greater or Equal" ),
			Spell::tr( "Never" )
		};

		QCheckBox * chkBlend = dlgCheck( vbox, Spell::tr( "Enable Blending" ) );
		chkBlend->setChecked( flags & 1 );

		/*
		 * The enabling/disabling of blend modes and test functions that was here until r4745
		 * disallows setting values that have been reported to otherwise work.
		 */
		QComboBox * cmbSrc = dlgCombo( vbox, Spell::tr( "Source Blend Mode" ), blendModes );
		cmbSrc->setCurrentIndex( flags >> 1 & 0x0f );

		QComboBox * cmbDst = dlgCombo( vbox, Spell::tr( "Destination Blend Mode" ), blendModes );
		cmbDst->setCurrentIndex( flags >> 5 & 0x0f );

		QCheckBox * chkTest = dlgCheck( vbox, Spell::tr( "Enable Testing" ) );
		chkTest->setChecked( flags & ( 1 << 9 ) );

		QComboBox * cmbTest = dlgCombo( vbox, Spell::tr( "Alpha Test Function" ), testModes );
		cmbTest->setCurrentIndex( flags >> 10 & 0x07 );

		QSpinBox * spnTest = dlgSpin( vbox, Spell::tr( "Alpha Test Threshold" ), 0x00, 0xff );
		spnTest->setValue( nif->get<int>( nif->getBlock( index ), "Threshold" ) );

		QCheckBox * chkSort = dlgCheck( vbox, Spell::tr( "No Sorter" ) );
		chkSort->setChecked( ( flags & 0x2000 ) != 0 );

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = flags & 0xfffe;

			if ( chkBlend->isChecked() ) {
				flags |= 1;
			}

			flags = ( flags & 0xffe1 ) | ( cmbSrc->currentIndex() << 1 );
			flags = ( flags & 0xfe1f ) | ( cmbDst->currentIndex() << 5 );

			flags = flags & 0xe1ff;

			if ( chkTest->isChecked() ) {
				flags |= 0x0200;
			}

			flags = ( flags & 0xe3ff ) | ( cmbTest->currentIndex() << 10 );
			nif->set<int>( nif->getBlock( index ), "Threshold", spnTest->value() );

			flags = ( flags & 0xdfff ) | ( chkSort->isChecked() ? 0x2000 : 0 );

			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a Node
	void nodeFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkHidden = dlgCheck( vbox, Spell::tr( "Hidden" ) );
		chkHidden->setChecked( flags & 1 );

		QStringList collideModes{
			Spell::tr( "None" ),
			Spell::tr( "Triangles" ),
			Spell::tr( "Bounding Box" ),
			Spell::tr( "Continue" )
		};

		QComboBox * cmbCollision = dlgCombo( vbox, Spell::tr( "Collision Detection" ), collideModes );
		cmbCollision->setCurrentIndex( flags >> 1 & 3 );

		QCheckBox * chkSkin = dlgCheck( vbox, Spell::tr( "Skin Influence" ) );
		chkSkin->setChecked( !( flags & 8 ) );

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfffe ) | ( chkHidden->isChecked() ? 1 : 0 );
			flags = ( flags & 0xfff9 ) | ( cmbCollision->currentIndex() << 1 );
			flags = ( flags & 0xfff7 ) | ( chkSkin->isChecked() ? 0 : 8 );
			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a Controller
	void controllerFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkActive = dlgCheck( vbox, Spell::tr( "Active" ) );
		chkActive->setChecked( flags & 8 );

		QStringList loopModes{
			Spell::tr( "Cycle" ),
			Spell::tr( "Reverse" ),
			Spell::tr( "Clamp" )
		};

		QComboBox * cmbLoop = dlgCombo( vbox, Spell::tr( "Loop Mode" ), loopModes );
		cmbLoop->setCurrentIndex( flags >> 1 & 3 );

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfff7 ) | ( chkActive->isChecked() ? 8 : 0 );
			flags = ( flags & 0xfff9 ) | ( cmbLoop->currentIndex() << 1 );
			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a bhkRigidBody
	void bodyFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout( &dlg );

		QCheckBox * chkLinked = dlgCheck( vbox, Spell::tr( "Linked" ) );
		chkLinked->setChecked( flags & 0x80 );
		QCheckBox * chkNoCol = dlgCheck( vbox, Spell::tr( "No Collision" ) );
		chkNoCol->setChecked( flags & 0x40 );
		QCheckBox * chkScaled = dlgCheck( vbox, Spell::tr( "Scaled" ) );
		chkScaled->setChecked( flags & 0x20 );

		QSpinBox * spnPartNo = dlgSpin( vbox, Spell::tr( "Part Number" ), 0, 0x1f, chkLinked );
		spnPartNo->setValue( flags & 0x1f );

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0x7f ) | ( chkLinked->isChecked() ? 0x80 : 0 );
			flags = ( flags & 0xbf ) | ( chkNoCol->isChecked() ? 0x40 : 0 );
			flags = ( flags & 0xdf ) | ( chkScaled->isChecked() ? 0x20 : 0 );
			flags = ( flags & 0xe0 ) | ( chkLinked->isChecked() ? spnPartNo->value() : 0 );
			nif->set<int>( index, flags );
			nif->set<int>( index.parent(), "Col Filter Copy", flags );
		}
	}

	//! Set flags on a Mesh
	void shapeFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkHidden = dlgCheck( vbox, Spell::tr( "Hidden" ) );
		chkHidden->setChecked( flags & 0x01 );

		QStringList collideModes{
			Spell::tr( "None" ),
			Spell::tr( "Triangles" ),
			Spell::tr( "Bounding Box" ),
			Spell::tr( "Continue" )
		};


		QComboBox * cmbCollision = dlgCombo( vbox, Spell::tr( "Collision Detection" ), collideModes );
		cmbCollision->setCurrentIndex( flags >> 1 & 3 );

		QCheckBox * chkShadow = nullptr;

		if ( nif->checkVersion( 0x04000002, 0x04000002 ) ) {
			chkShadow = dlgCheck( vbox, Spell::tr( "Shadow" ) );
			chkShadow->setChecked( flags & 0x40 );
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfffe ) | ( chkHidden->isChecked() ? 0x01 : 0 );
			flags = ( flags & 0xfff9 ) | ( cmbCollision->currentIndex() << 1 );

			if ( chkShadow ) {
				flags = ( flags & 0xffbf ) | ( chkShadow->isChecked() ? 0x40 : 0 );
			}

			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a ZBufferProperty
	void zbufferFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkEnable = dlgCheck( vbox, Spell::tr( "Enable Z Buffer" ) );
		chkEnable->setChecked( flags & 1 );

		QCheckBox * chkROnly = dlgCheck( vbox, Spell::tr( "Z Buffer Read Only" ) );
		chkROnly->setChecked( ( flags & 2 ) == 0 );

		// ALWAYS and NEVER are swapped, otherwise values match glDepthFunc
		QStringList compareFunc{
			Spell::tr( "Always" ),           // 0
			Spell::tr( "Less" ),             // 1
			Spell::tr( "Equal" ),            // 2
			Spell::tr( "Less or Equal" ),    // 3
			Spell::tr( "Greater" ),          // 4
			Spell::tr( "Not Equal" ),        // 5
			Spell::tr( "Greater or Equal" ), // 6
			Spell::tr( "Never" )            // 7
		};

		QComboBox * cmbFunc = dlgCombo( vbox, Spell::tr( "Z Buffer Test Function" ), compareFunc, chkEnable );

		if ( nif->checkVersion( 0x0401000C, 0x14000005 ) )
			cmbFunc->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Function" ) );
		else
			cmbFunc->setCurrentIndex( ( flags >> 2 ) & 0x07 );

		QCheckBox * setFlags = nullptr;

		if ( nif->checkVersion( 0x0401000C, 0x14000005 ) ) {
			setFlags = dlgCheck( vbox, Spell::tr( "Set Flags also" ) );
			setFlags->setChecked( flags > 3 );
		}

		dlgButtons( &dlg, vbox );

		// TODO: check which is used for 4.1.0.12 < x < 20.0.0.5 if flags
		// and function conflict... perhaps set flags regardless?
		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfffe ) | ( chkEnable->isChecked() ? 1 : 0 );
			flags = ( flags & 0xfffd ) | ( chkROnly->isChecked() ? 0 : 2 );

			if ( nif->checkVersion( 0x0401000C, 0x14000005 ) ) {
				nif->set<int>( nif->getBlock( index ), "Function", cmbFunc->currentIndex() );
			}

			if ( nif->checkVersion( 0x14010003, 0 ) || ( setFlags != 0 && setFlags->isChecked() ) ) {
				flags = ( flags & 0xffe3 ) | ( cmbFunc->currentIndex() << 2 );
			}

			nif->set<int>( index, flags );
		}
	}

	//! Set BSX flags
	void bsxFlags( NifModel * nif, const QModelIndex & index )
	{
		quint32 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout( &dlg );

		QStringList flagNames{
			Spell::tr( "Animated" ),            // 1
			Spell::tr( "Havok" ),               // 2
			Spell::tr( "Ragdoll" ),             // 4
			Spell::tr( "Complex" ),             // 8
			Spell::tr( "Addon" ),               // 16
			Spell::tr( "Editor Marker" ),       // 32
			Spell::tr( "Dynamic" ),             // 64
			Spell::tr( "Articulated" ),         // 128
			Spell::tr( "Needs Transform Updates" ),  // 256
			Spell::tr( "External Emit" ),            // 512
			//Spell::tr( "Magic Shader Particles" ), // 1024
			//Spell::tr( "Lights" ),                 // 2048
			//Spell::tr( "Breakable" ),              // 4096
			//Spell::tr( "Searched Breakable (Runtime Only?)" ) // 8192
		};

		QList<QCheckBox *> chkBoxes;
		int x = 0;
		for ( const QString& flagName : flagNames ) {
			chkBoxes << dlgCheck( vbox, QString( "%1: %2 (%3)" ).arg( Spell::tr( "Bit %1" ).arg( x ) ).arg( flagName ).arg( 1 << x ) );
			chkBoxes.last()->setChecked( flags & ( 1 << x ) );
			x++;
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			x = 0;
			for ( QCheckBox * chk : chkBoxes ) {
				flags = ( flags & ( ~( 1 << x ) ) ) | ( chk->isChecked() ? 1 << x : 0 );
				x++;
			}
			nif->set<int>( index, flags );
		}
	}

	//! Set NiAVObject flags
	void niavFlags( NifModel * nif, const QModelIndex & index )
	{
		quint32 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout( &dlg );

		QStringList flagNames{
			Spell::tr( "Hidden" ),                          // 1
			Spell::tr( "Selective Update" ),                // 2
			Spell::tr( "Selective Update Transforms" ),     // 4
			Spell::tr( "Selective Update Controller" ),     // 8
			Spell::tr( "Selective Update Rigid" ),          // 16
			Spell::tr( "Display (UI?) Object" ),            // 32
			Spell::tr( "Disable Sorting" ),                 // 64
			Spell::tr( "Sel. Upd. Transforms Override" ),   // 128
			Spell::tr( "" ),                                // 256
			Spell::tr( "Save External Geom Data" ),         // 512
			Spell::tr( "No Decals" ),                       // 1024
			Spell::tr( "Always Draw" ),                     // 2048
			Spell::tr( "Mesh LOD" ),                        // 4096
			Spell::tr( "Fixed Bound" ),                     // 8192
			Spell::tr( "Top Fade Node" ),                   // 16384
			Spell::tr( "Ignore Fade" ),                     // 32768
			Spell::tr( "No Anim Sync (X)" ),                // 65536
			Spell::tr( "No Anim Sync (Y)" ),                // 1 << 17
			Spell::tr( "No Anim Sync (Z)" ),                // 1 << 18
			Spell::tr( "No Anim Sync (S)" ),                // 1 << 19
			Spell::tr( "No Dismember" ),                    // 1 << 20
			Spell::tr( "No Dismember Validity" ),           // 1 << 21
			Spell::tr( "Render Use" ),                      // 1 << 22
			Spell::tr( "Materials Applied" ),               // 1 << 23
			Spell::tr( "High Detail" ),                     // 1 << 24
			Spell::tr( "Force Update" ),                    // 1 << 25
			Spell::tr( "Pre-Processed Node" ),              // 1 << 26
			Spell::tr( "Bit 27" ),                          // 1 << 27
			Spell::tr( "Bit 28" ),                          // 1 << 28
			Spell::tr( "Bit 29" ),                          // 1 << 29
			Spell::tr( "Bit 30" ),                          // 1 << 30
			Spell::tr( "Bit 31" ),                          // 1 << 31

		};

		QList<QCheckBox *> chkBoxes;
		int x = 0;
		for ( const QString& flagName : flagNames ) {
			chkBoxes << dlgCheck( vbox, QString( "%1: %2 (%3)" ).arg( Spell::tr( "Bit %1" ).arg( x ) )
								  .arg( flagName ).arg( (uint)(1 << x) ) );
			chkBoxes.last()->setChecked( flags & (1 << x) );
			x++;
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			x = 0;
			for ( QCheckBox * chk : chkBoxes ) {
				flags = (flags & (~(1 << x))) | (chk->isChecked() ? 1 << x : 0);
				x++;
			}
			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a BillboardNode
	void billboardFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		if ( !flags )
			flags = 0x8;

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkHidden = dlgCheck( vbox, Spell::tr( "Hidden" ) );
		chkHidden->setChecked( flags & 1 );

		QStringList collideModes{
			Spell::tr( "None" ),                // 0
			Spell::tr( "Triangles" ),           // 2
			Spell::tr( "Bounding Box" ),        // 4
			Spell::tr( "Continue" ),            // 6
		};

		QStringList billboardModes{
			Spell::tr( "Always Face Camera" ),  // 0
			Spell::tr( "Rotate About Up" ),     // 32
			Spell::tr( "Rigid Face Camera" ),   // 64
			Spell::tr( "Always Face Center" )   // 96
		};

		QComboBox * cmbCollision = dlgCombo( vbox, Spell::tr( "Collision Detection" ), collideModes );
		cmbCollision->setCurrentIndex( flags >> 1 & 3 );

		QComboBox * cmbMode = dlgCombo( vbox, Spell::tr( "Billboard Mode" ), billboardModes );

		// Billboard Mode is an enum as of 10.1.0.0
		if ( nif->checkVersion( 0x0A010000, 0 ) ) {
			// this value doesn't exist before 10.1.0.0
			// ROTATE_ABOUT_UP2 is too hard to put in and possibly meaningless
			cmbMode->addItem( Spell::tr( "Rigid Face Center" ) );
			cmbMode->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Billboard Mode" ) );
		} else {
			cmbMode->setCurrentIndex( flags >> 5 & 3 );
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfffe ) | ( chkHidden->isChecked() ? 1 : 0 );
			flags = ( flags & 0xfff9 ) | ( cmbCollision->currentIndex() << 1);

			if ( nif->checkVersion( 0x0A010000, 0 ) ) {
				nif->set<int>( nif->getBlock( index ), "Billboard Mode", cmbMode->currentIndex() );
			} else {
				flags = ( flags & 0xff9f ) | ( cmbMode->currentIndex() << 5 );
				flags = ( flags & 0xfff7 ) | 8; // seems to always be set but has no known effect
			}

			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a StencilProperty
	void stencilFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkEnable = dlgCheck( vbox, Spell::tr( "Stencil Enable" ) );

		QStringList stencilActions{
			Spell::tr( "Keep" ),      // 0
			Spell::tr( "Zero" ),      // 1
			Spell::tr( "Replace" ),   // 2
			Spell::tr( "Increment" ), // 3
			Spell::tr( "Decrement" ), // 4
			Spell::tr( "Invert" )     // 5
		};


		QComboBox * cmbFail = dlgCombo( vbox, Spell::tr( "Fail Action" ), stencilActions );

		QComboBox * cmbZFail = dlgCombo( vbox, Spell::tr( "Z Fail Action" ), stencilActions );

		QComboBox * cmbPass = dlgCombo( vbox, Spell::tr( "Pass Action" ), stencilActions );

		QStringList drawModes{
			Spell::tr( "Counter clock wise or Both" ), // 0
			Spell::tr( "Draw counter clock wise" ),    // 1
			Spell::tr( "Draw clock wise" ),            // 2
			Spell::tr( "Draw Both" )                   // 3
		};

		QComboBox * cmbDrawMode = dlgCombo( vbox, Spell::tr( "Draw Mode" ), drawModes );

		// Appears to match glStencilFunc
		QStringList compareFunc{
			Spell::tr( "Never" ),            // 0
			Spell::tr( "Less" ),             // 1
			Spell::tr( "Equal" ),            // 2
			Spell::tr( "Less or Equal" ),    // 3
			Spell::tr( "Greater" ),          // 4
			Spell::tr( "Not Equal" ),        // 5
			Spell::tr( "Greater or Equal" ), // 6
			Spell::tr( "Always" )            // 7
		};          

		QComboBox * cmbFunc = dlgCombo( vbox, Spell::tr( "Stencil Function" ), compareFunc );

		// prior to 20.1.0.3 flags itself appears unused; after 10.0.1.2 and until 20.1.0.3 it is not present
		// 20.0.0.5 is the last version with these values
		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			// set based on Stencil Enabled, Stencil Function, Fail Action, Z Fail Action, Pass Action, Draw Mode
			// Possibly include Stencil Ref and Stencil Mask except they don't seem to ever vary from the default
			chkEnable->setChecked( nif->get<bool>( nif->getBlock( index ), "Stencil Enabled" ) );
			cmbFail->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Fail Action" ) );
			cmbZFail->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Z Fail Action" ) );
			cmbPass->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Pass Action" ) );
			cmbDrawMode->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Draw Mode" ) );
			cmbFunc->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Stencil Function" ) );
		} else {
			// set based on flags itself
			chkEnable->setChecked( flags & 1 );
			cmbFail->setCurrentIndex( flags >> 1 & 7 );
			cmbZFail->setCurrentIndex( flags >> 4 & 7 );
			cmbPass->setCurrentIndex( flags >> 7 & 7 );
			cmbDrawMode->setCurrentIndex( flags >> 10 & 3 );
			cmbFunc->setCurrentIndex( flags >> 12 & 7 );
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			if ( nif->checkVersion( 0, 0x14000005 ) ) {
				nif->set<bool>( nif->getBlock( index ), "Stencil Enabled", chkEnable->isChecked() );
				nif->set<int>( nif->getBlock( index ), "Fail Action", cmbFail->currentIndex() );
				nif->set<int>( nif->getBlock( index ), "Z Fail Action", cmbZFail->currentIndex() );
				nif->set<int>( nif->getBlock( index ), "Pass Action", cmbPass->currentIndex() );
				nif->set<int>( nif->getBlock( index ), "Draw Mode", cmbDrawMode->currentIndex() );
				nif->set<int>( nif->getBlock( index ), "Stencil Function", cmbFunc->currentIndex() );
			} else {
				flags = ( flags & 0xfffe ) | ( chkEnable->isChecked() ? 1 : 0 );
				flags = ( flags & 0xfff1 ) | ( cmbFail->currentIndex() << 1 );
				flags = ( flags & 0xff8f ) | ( cmbZFail->currentIndex() << 4 );
				flags = ( flags & 0xfc7f ) | ( cmbPass->currentIndex() << 7 );
				flags = ( flags & 0xf3ff ) | ( cmbDrawMode->currentIndex() << 10 );
				flags = ( flags & 0x8fff ) | ( cmbFunc->currentIndex() << 12 );
				nif->set<int>( index, flags );
			}
		}
	}

	//! Set flags on a VertexColorProperty
	void vertexColorFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		// For versions < 20.1.0.3 (<= 20.0.0.5), give option to set flags regardless
		QCheckBox * setFlags = nullptr;

		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			setFlags = dlgCheck( vbox, Spell::tr( "Set Flags also" ) );
			setFlags->setChecked( flags != 0 );
		}

		QStringList lightMode{
			Spell::tr( "Emissive" ),
			Spell::tr( "Emissive + Ambient + Diffuse" )
		};

		QComboBox * cmbLight = dlgCombo( vbox, Spell::tr( "Lighting Mode" ), lightMode );

		QStringList vertMode{
			Spell::tr( "Source Ignore" ),
			Spell::tr( "Source Emissive" ),
			Spell::tr( "Source Ambient/Diffuse" )
		};

		QComboBox * cmbVert = dlgCombo( vbox, Spell::tr( "Vertex Mode" ), vertMode );

		// Use enums in preference to flags since they probably have a higher priority
		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			cmbLight->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Lighting Mode" ) );
			cmbVert->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Vertex Mode" ) );
		} else {
			cmbLight->setCurrentIndex( flags >> 3 & 1 );
			cmbVert->setCurrentIndex( flags >> 4 & 3 );
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			if ( nif->checkVersion( 0, 0x14000005 ) ) {
				nif->set<int>( nif->getBlock( index ), "Lighting Mode", cmbLight->currentIndex() );
				nif->set<int>( nif->getBlock( index ), "Vertex Mode", cmbVert->currentIndex() );
			}

			if ( nif->checkVersion( 0x14010003, 0 ) || ( setFlags != 0 && setFlags->isChecked() ) ) {
				flags = ( flags & 0xfff7 ) | ( cmbLight->currentIndex() << 3 );
				flags = ( flags & 0xffcf ) | ( cmbVert->currentIndex() << 4 );
				nif->set<int>( index, flags );
			}
		}
	}

	//! Set flags on a MaterialColorController
	void matColControllerFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QCheckBox * chkActive = dlgCheck( vbox, Spell::tr( "Active" ) );
		chkActive->setChecked( flags & 8 );

		QStringList loopModes{
			Spell::tr( "Cycle" ), Spell::tr( "Reverse" ), Spell::tr( "Clamp" )
		};

		QComboBox * cmbLoop = dlgCombo( vbox, Spell::tr( "Loop Mode" ), loopModes );
		cmbLoop->setCurrentIndex( flags >> 1 & 3 );

		QStringList targetColor{
			Spell::tr( "Ambient" ), Spell::tr( "Diffuse" ),
			Spell::tr( "Specular" ), Spell::tr( "Emissive" )
		};

		QComboBox * cmbColor = dlgCombo( vbox, Spell::tr( "Target Color" ), targetColor );

		// Target Color enum exists as of 10.1.0.0
		if ( nif->checkVersion( 0x0A010000, 0 ) ) {
			cmbColor->setCurrentIndex( nif->get<int>( nif->getBlock( index ), "Target Color" ) );
		} else {
			cmbColor->setCurrentIndex( flags >> 4 & 3 );
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0xfff7 ) | ( chkActive->isChecked() ? 8 : 0 );
			flags = ( flags & 0xfff9 ) | ( cmbLoop->currentIndex() << 1 );

			if ( nif->checkVersion( 0x0A010000, 0 ) ) {
				nif->set<int>( nif->getBlock( index ), "Target Color", cmbColor->currentIndex() );
			} else {
				flags = ( flags & 0xffcf ) | ( cmbColor->currentIndex() << 4 );
			}

			nif->set<int>( index, flags );
		}
	}

	//! Set flags on a TexDesc struct
	void texDescFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QSpinBox * spnUVIndex = dlgSpin( vbox, Spell::tr( "UV Index" ), 0, 0xFF );
		spnUVIndex->setValue( flags & 0x00FF );

		QStringList clampModes{
			Spell::tr( "Clamp Both" ),
			Spell::tr( "Clamp S Wrap T" ),
			Spell::tr( "Wrap S Clamp T" ),
			Spell::tr( "Wrap Both" )
		};

		QComboBox * cmbClamp = dlgCombo( vbox, Spell::tr( "Clamp Mode" ), clampModes );
		cmbClamp->setCurrentIndex( ( flags & 0xF000 ) >> 0x0C );

		QStringList filterModes{
			Spell::tr( "FILTER_NEAREST" ),
			Spell::tr( "FILTER_BILERP" ),
			Spell::tr( "FILTER_TRILERP" ),
			Spell::tr( "FILTER_NEAREST_MIPNEAREST" ),
			Spell::tr( "FILTER_NEAREST_MIPLERP" ),
			Spell::tr( "FILTER_BILERP_MIPNEAREST" )
		};

		QComboBox * cmbFilter = dlgCombo( vbox, Spell::tr( "Filter Mode" ), filterModes );
		cmbFilter->setCurrentIndex( ( flags & 0x0F00 ) >> 0x08 );

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			flags = ( flags & 0x0FFF ) | ( cmbClamp->currentIndex() << 0x0C );
			flags = ( flags & 0xF0FF ) | ( cmbFilter->currentIndex() << 0x08 );
			flags = ( flags & 0xFF00 ) | spnUVIndex->value();
			nif->set<int>( index, flags );
		}
	}

	//! Add a checkbox to a dialog
	/*!
	 * \param vbox Vertical box layout to add the checkbox to
	 * \param name The name to give the checkbox
	 * \param chk A checkbox that enables or disables this checkbox
	 * \return A pointer to the checkbox
	 */
	QCheckBox * dlgCheck( QVBoxLayout * vbox, const QString & name, QCheckBox * chk = nullptr )
	{
		QCheckBox * box = new QCheckBox( name );
		vbox->addWidget( box );

		if ( chk ) {
			QObject::connect( chk, &QCheckBox::toggled, box, &QCheckBox::setEnabled );
			box->setEnabled( chk->isChecked() );
		}

		return box;
	}

	//! Add a combobox to a dialog
	/*!
	 * \param vbox Vertical box layout to add the combobox to
	 * \param name The name to give the combobox
	 * \param items The items to add to the combobox
	 * \param chk A checkbox that enables or disables this combobox
	 * \return A pointer to the combobox
	 */
	QComboBox * dlgCombo( QVBoxLayout * vbox, const QString & name, QStringList items, QCheckBox * chk = nullptr )
	{
		vbox->addWidget( new QLabel( name ) );
		QComboBox * cmb = new QComboBox;
		vbox->addWidget( cmb );
		cmb->addItems( items );

		if ( chk ) {
			QObject::connect( chk, &QCheckBox::toggled, cmb, &QComboBox::setEnabled );
			cmb->setEnabled( chk->isChecked() );
		}

		return cmb;
	}

	//! Add a spinbox to a dialog
	/*!
	 * \param vbox Vertical box layout to add the spinbox to
	 * \param name The name to give the spinbox
	 * \param min The minimum value of the spinbox
	 * \param max The maximum value of the spinbox
	 * \param chk A checkbox that enables or disables this spinbox
	 * \return A pointer to the spinbox
	 */
	QSpinBox * dlgSpin( QVBoxLayout * vbox, const QString & name, int min, int max, QCheckBox * chk = nullptr )
	{
		vbox->addWidget( new QLabel( name ) );
		QSpinBox * spn = new QSpinBox;
		vbox->addWidget( spn );
		spn->setRange( min, max );

		if ( chk ) {
			QObject::connect( chk, &QCheckBox::toggled, spn, &QSpinBox::setEnabled );
			spn->setEnabled( chk->isChecked() );
		}

		return spn;
	}

	//! Add standard buttons to a dialog
	/*!
	 * \param dlg The dialog to add buttons to
	 * \param vbox Vertical box layout used by the dialog
	 */
	void dlgButtons( QDialog * dlg, QVBoxLayout * vbox )
	{
		QHBoxLayout * hbox = new QHBoxLayout;
		vbox->addLayout( hbox );

		QPushButton * btAccept = new QPushButton( Spell::tr( "Accept" ) );
		hbox->addWidget( btAccept );
		QObject::connect( btAccept, &QPushButton::clicked, dlg, &QDialog::accept );

		QPushButton * btReject = new QPushButton( Spell::tr( "Cancel" ) );
		hbox->addWidget( btReject );
		QObject::connect( btReject, &QPushButton::clicked, dlg, &QDialog::reject );
	}
};

REGISTER_SPELL( spEditFlags )


class spEditVertexDesc final : public spEditFlags
{
public:
	QString name() const override final { return Spell::tr( "Vertex Flags" ); }
	bool instant() const override final { return true; }
	QIcon icon() const override final { return QIcon( ":/img/flag" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->inherits( index.parent(), "BSTriShape" ) && nif->itemName( index ) == "Vertex Desc";
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		auto desc = nif->get<BSVertexDesc>( index );
		uint stream = nif->getUserVersion2();
		bool dynamic = nif->inherits( index.parent(), "BSDynamicTriShape" );

		QStringList flagNames {
			Spell::tr( "Vertex" ),	  // VA_POSITION = 0x0,
			Spell::tr( "UVs" ),		  // VA_TEXCOORD0 = 0x1,
			Spell::tr( "UVs 2" ),	  // VA_TEXCOORD1 = 0x2,
			Spell::tr( "Normals" ),	  // VA_NORMAL = 0x3,
			Spell::tr( "Tangents" ),  // VA_BINORMAL = 0x4,
			Spell::tr( "Colors" ),	  // VA_COLOR = 0x5,
			Spell::tr( "Skinned" ),	  // VA_SKINNING = 0x6,
			Spell::tr( "Land Data" ), // VA_LANDDATA = 0x7,
			Spell::tr( "Eye Data" ),  // VA_EYEDATA = 0x8,
			Spell::tr( "" ),          
			Spell::tr( "Full Precision" ) // 1 << 10
		};

		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		QList<QCheckBox *> chkBoxes;
		int x = 0;
		for ( const QString& flagName : flagNames ) {
			chkBoxes << dlgCheck( vbox, flagName );
			chkBoxes.last()->setChecked( desc.HasFlag( VertexAttribute(x) ) );
			// Hide unused attributes
			if ( x == 2 || x == 7 || x == 9 )
				chkBoxes.last()->setHidden( true );
			x++;
		}

		dlgButtons( &dlg, vbox );

		if ( dlg.exec() == QDialog::Accepted ) {
			x = 0;
			for ( QCheckBox * chk : chkBoxes ) {
				if ( chk->isChecked() )
					desc.SetFlag( VertexAttribute(x) );
				else
					desc.RemoveFlag( VertexAttribute(x) );
				x++;
			}

			// Make sure sizes and offsets in rest of vertexDesc are updated from flags
			desc.ResetAttributeOffsets( stream );
			if ( dynamic )
				desc.MakeDynamic();

			if ( nif->set<BSVertexDesc>( index, desc ) ) {
				auto iDataSize = nif->getIndex( index.parent(), "Data Size" );
				auto numVerts = nif->get<uint>( index.parent(), "Num Vertices" );
				auto numTris = nif->get<uint>( index.parent(), "Num Triangles" );

				if ( iDataSize.isValid() )
					nif->set<uint>( iDataSize, desc.GetVertexSize() * numVerts + 6 * numTris );

				nif->updateArray( index.parent(), "Vertex Data" );
			}

		}

		return index;
	}

};

REGISTER_SPELL( spEditVertexDesc )
