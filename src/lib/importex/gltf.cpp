#include "nifskope.h"
#include "data/niftypes.h"
#include "gl/glscene.h"
#include "gl/glnode.h"
#include "gl/gltools.h"
#include "gl/BSMesh.h"
#include "io/MeshFile.h"
#include "model/nifmodel.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <cmath>

#include <QApplication>
#include <QBuffer>
#include <QVector>
#include <QFileDialog>

#define tr( x ) QApplication::tr( x )

struct GltfStore
{
	// Block ID to list of gltfNodeID
	// BSGeometry may have 1-4 associated gltfNodeID to deal with LOD0-LOD3
	// NiNode will only have 1 gltfNodeID
	QMap<int, QVector<int>> nodes;
	// gltfJointID to gltfNodeID
	QMap<int, int> bones;
	// Bone names to gltfNodeID
	QMap<QString, int> boneNames;
	// gltfSkinID to BSMesh
	QMap<int, BSMesh*> skins;
	// Material Paths
	QStringList materials;

	QStringList errors;
};


void exportCreateInverseBoneMatrices(tinygltf::Model& model, QByteArray& bin, const BSMesh* bsmesh, int gltfSkinID, GltfStore& gltf)
{
	auto bufferViewIndex = model.bufferViews.size();
	auto acc = tinygltf::Accessor();
	acc.bufferView = bufferViewIndex;
	acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	acc.count = bsmesh->boneTransforms.size();
	acc.type = TINYGLTF_TYPE_MAT4;
	model.accessors.push_back(acc);

	tinygltf::BufferView view;
	view.buffer = 0;
	view.byteOffset = bin.size();
	view.byteLength = acc.count * tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type);
	model.bufferViews.push_back(view);

	model.skins[gltfSkinID].inverseBindMatrices = bufferViewIndex;

	for ( const auto& b : bsmesh->boneTransforms ) {
		bin.append(reinterpret_cast<const char*>(b.toMatrix4().data()), sizeof(Matrix4));
	}
}

bool exportCreateNodes(const NifModel* nif, const Scene* scene, tinygltf::Model& model, QByteArray& bin, GltfStore& gltf)
{
	int gltfNodeID = 0;
	int gltfSkinID = 0;
	auto& nodes = scene->nodes.list();
	for ( const auto node : nodes ) {
		if ( !node )
			continue;

		auto nodeId = node->id();
		auto iBlock = nif->getBlockIndex(nodeId);
		if ( nif->blockInherits(iBlock, { "NiNode", "BSGeometry" }) ) {
			auto gltfNode = tinygltf::Node();
			auto mesh = static_cast<BSMesh*>(node);
			bool hasGPULODs = false;
			bool isBSGeometry = nif->blockInherits(iBlock, "BSGeometry") && mesh;
			// Create extra nodes for GPU LODs
			int createdNodes = 1;
			if ( isBSGeometry ) {
				if ( !mesh->materialPath.isEmpty() && !gltf.materials.contains(mesh->materialPath) ) {
					gltf.materials << mesh->materialPath;
				}
				hasGPULODs = mesh->gpuLODs.size() > 0;
				createdNodes = mesh->meshCount();
				if ( hasGPULODs )
					createdNodes = mesh->gpuLODs.size() + 1;
			}

			for ( int j = 0; j < createdNodes; j++ ) {
				// Fill nodes map
				gltf.nodes[nodeId].append(gltfNodeID);

				gltfNode.name = node->getName().toStdString();
				if ( isBSGeometry ) {
					gltfNode.name += ":LOD" + std::to_string(j);
					// Skins
					if ( mesh->skinID > -1 && mesh->weightsUNORM.size() > 0 ) {
						gltfNode.skin = gltfSkinID;
						gltf.skins[gltfSkinID] = mesh;
						gltfSkinID++;
					}
				}

				// Rotate the root NiNode for glTF Y-Up
				Matrix rot;
				auto& trans = node->localTrans();
				if ( gltfNodeID == 0 ) {
					rot = trans.rotation.toYUp();
				} else {
					rot = trans.rotation;
				}
				auto quat = rot.toQuat();
				gltfNode.translation = { trans.translation[0], trans.translation[1], trans.translation[2] };
				gltfNode.rotation = { quat[1], quat[2], quat[3], quat[0] };
				gltfNode.scale = { trans.scale, trans.scale, trans.scale };

				std::map<std::string, tinygltf::Value> extras;
				auto links = nif->getChildLinks(nif->getBlockNumber(node->index()));
				for ( const auto link : links ) {
					auto idx = nif->getBlockIndex(link);
					if ( nif->blockInherits(idx, "BSShaderProperty") ) {
						extras["Material Path"] = tinygltf::Value(nif->get<QString>(idx, "Name").toStdString());
					} else if ( nif->blockInherits(idx, "NiIntegerExtraData") ) {
						auto key = QString("%1:%2").arg(nif->itemName(idx)).arg(nif->get<QString>(idx, "Name"));
						extras[key.toStdString()] = tinygltf::Value(nif->get<int>(idx, "Integer Data"));
					}
				}

				auto flags = nif->get<int>(iBlock, "Flags");
				extras["Flags"] = tinygltf::Value(flags);
				if ( isBSGeometry )
					extras["Has GPU LODs"] = tinygltf::Value(hasGPULODs);
				gltfNode.extras = tinygltf::Value(extras);

				model.nodes.push_back(gltfNode);
				gltfNodeID++;
			}

		}
	}

	// Add child nodes after first pass
	for ( int i = 0; i < nif->getBlockCount(); i++ ) {
		auto iBlock = nif->getBlockIndex(i);
	
		if ( nif->blockInherits(iBlock, "NiNode") ) {
			auto children = nif->getChildLinks(i);
			for ( const auto& child : children ) {
				auto nodes = gltf.nodes.value(child, {});
				for ( const auto& node : nodes ) {
					model.nodes[gltf.nodes[i][0]].children.push_back(node);
				}

			}
		}
	}

	// SKINNING

	bool hasSkeleton = false;
	for ( const auto shape : scene->shapes ) {
		if ( !shape )
			continue;
		auto mesh = static_cast<BSMesh*>(shape);
		if ( mesh->boneNames.size() > 0 ) {
			hasSkeleton = true;
			break;
		}
	}
	if ( hasSkeleton ) {
		for ( const auto& skin : gltf.skins ) {
			auto gltfSkin = tinygltf::Skin();
			model.skins.push_back(gltfSkin);
		}

		for ( const auto shape : scene->shapes ) {
			if ( !shape )
				continue;

			auto nodeId = shape->id();
			auto iBlock = nif->getBlockIndex(nodeId);
			if ( nif->blockInherits(iBlock, "BSGeometry") ) {
				auto mesh = static_cast<BSMesh*>(shape);
				if ( mesh && mesh->boneNames.size() > 0 ) {
					// Map Bones
					int nameIndex = 0;
					for ( const auto& name : mesh->boneNames ) {
						auto it = std::find_if(model.nodes.begin(), model.nodes.end(), [&](const tinygltf::Node& n) {
							return n.name == name.toStdString();
						});

						int gltfNodeID = (it != model.nodes.end()) ? it - model.nodes.begin() : -1;
						if ( gltfNodeID > -1 ) {
							gltf.bones[nameIndex] = gltfNodeID;
							gltf.boneNames[name] = gltfNodeID;
						}

						nameIndex++;
					}
				}
			}
		}

		// Find COM or COM_Twin first if available
		auto it = std::find_if(model.nodes.begin(), model.nodes.end(), [&](const tinygltf::Node& n) {
			return n.name == "COM_Twin" || n.name == "COM";
		});

		int skeletonRoot = (it != model.nodes.end()) ? it - model.nodes.begin() : -1;
		if ( skeletonRoot > -1 ) {
			for ( const auto& node : model.nodes ) {
				auto id = node.skin;
				if ( id > -1 ) {
					auto mesh = gltf.skins.value(id, nullptr);
					if ( mesh ) {
						model.skins[id].skeleton = skeletonRoot;
						for ( const auto& boneName : mesh->boneNames ) {
							auto gltfJointID = gltf.boneNames.value(boneName, -1);
							if ( gltfJointID )
								model.skins[id].joints.push_back(gltfJointID);
						}

						exportCreateInverseBoneMatrices(model, bin, mesh, id, gltf);
					}
				}
			}
		} else {
			gltf.errors << "Skeletal mesh requires COM or COM_Twin for export. You may copy/paste the COM NiNode branch of the mesh's skeleton.nif before export.";
			return false;
		}
	}

	return true;
}


void exportCreatePrimitive(tinygltf::Model& model, QByteArray& bin, std::shared_ptr<MeshFile> mesh, tinygltf::Primitive& prim, std::string attr,
						   int count, int componentType, int type, quint32& attributeIndex, GltfStore& gltf)
{
	if ( count < 1 )
		return;

	auto acc = tinygltf::Accessor();
	acc.bufferView = attributeIndex;
	acc.componentType = componentType;
	acc.count = count;
	acc.type = type;

	// Min/Max bounds
	// TODO: Utility function in niftypes
	if ( attr == "POSITION" ) {
		Vector3 max{ -INFINITY, -INFINITY, -INFINITY };
		Vector3 min{ INFINITY, INFINITY, INFINITY };

		for ( const auto& v : mesh->positions ) {
			if ( v[0] > max[0] )
				max[0] = v[0];
			if ( v[0] < min[0] )
				min[0] = v[0];

			if ( v[1] > max[1] )
				max[1] = v[1];
			if ( v[1] < min[1] )
				min[1] = v[1];

			if ( v[2] > max[2] )
				max[2] = v[2];
			if ( v[2] < min[2] )
				min[2] = v[2];
		}

		Q_ASSERT(min[0] != INFINITY);
		Q_ASSERT(min[1] != INFINITY);
		Q_ASSERT(min[2] != INFINITY);

		Q_ASSERT(max[0] != -INFINITY);
		Q_ASSERT(max[1] != -INFINITY);
		Q_ASSERT(max[2] != -INFINITY);

		acc.minValues.push_back(min[0]);
		acc.minValues.push_back(min[1]);
		acc.minValues.push_back(min[2]);

		acc.maxValues.push_back(max[0]);
		acc.maxValues.push_back(max[1]);
		acc.maxValues.push_back(max[2]);
	}

	prim.mode = TINYGLTF_MODE_TRIANGLES;
	prim.attributes[attr] = attributeIndex++;

	model.accessors.push_back(acc);

	auto size = tinygltf::GetComponentSizeInBytes(acc.componentType);

	tinygltf::BufferView view;
	view.buffer = 0;

	auto pad = bin.size() % size;
	for ( int i = 0; i < pad; i++ ) {
		bin.append("\xFF");
	}
	view.byteOffset = bin.size();
	view.byteLength = count * size * tinygltf::GetNumComponentsInType(acc.type);
	view.target = TINYGLTF_TARGET_ARRAY_BUFFER;

	bin.reserve(bin.size() + view.byteLength);
	// TODO: Refactoring BSMesh to std::vector for aligned allocators
	// would bring incompatibility with Shape superclass and take a larger refactor.
	// So, do this for now.
	if ( attr == "POSITION" ) {
		for ( const auto& v : mesh->positions ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
			bin.append(reinterpret_cast<const char*>(&v[2]), sizeof(v[2]));
		}
	} else if ( attr == "NORMAL" ) {
		for ( const auto& v : mesh->normals ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
			bin.append(reinterpret_cast<const char*>(&v[2]), sizeof(v[2]));
		}
	} else if ( attr == "TANGENT" ) {
		for ( const auto& v : mesh->tangentsBasis ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
			bin.append(reinterpret_cast<const char*>(&v[2]), sizeof(v[2]));
			bin.append(reinterpret_cast<const char*>(&v[3]), sizeof(v[3]));
		}
	} else if ( attr == "TEXCOORD_0" ) {
		for ( const auto& v : mesh->coords[0] ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
		}
	} else if ( attr == "TEXCOORD_1" ) {
		for ( const auto& v : mesh->coords[1] ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
		}
	} else if ( attr == "COLOR_0" ) {
		for ( const auto& v : mesh->colors ) {
			bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
			bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
			bin.append(reinterpret_cast<const char*>(&v[2]), sizeof(v[2]));
			bin.append(reinterpret_cast<const char*>(&v[3]), sizeof(v[3]));
		}
	} else if ( attr == "WEIGHTS_0" ) {
		for ( const auto& v : mesh->weights ) {
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[0].weight), sizeof(v.weightsUNORM[0].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[1].weight), sizeof(v.weightsUNORM[1].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[2].weight), sizeof(v.weightsUNORM[2].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[3].weight), sizeof(v.weightsUNORM[3].weight));
		}
	} else if ( attr == "WEIGHTS_1" ) {
		for ( const auto& v : mesh->weights ) {
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[4].weight), sizeof(v.weightsUNORM[4].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[5].weight), sizeof(v.weightsUNORM[5].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[6].weight), sizeof(v.weightsUNORM[6].weight));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[7].weight), sizeof(v.weightsUNORM[7].weight));
		}
	} else if ( attr == "JOINTS_0" ) {
		for ( const auto& v : mesh->weights ) {
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[0].bone), sizeof(v.weightsUNORM[0].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[1].bone), sizeof(v.weightsUNORM[1].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[2].bone), sizeof(v.weightsUNORM[2].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[3].bone), sizeof(v.weightsUNORM[3].bone));
		}
	} else if ( attr == "JOINTS_1" ) {
		for ( const auto& v : mesh->weights ) {
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[4].bone), sizeof(v.weightsUNORM[4].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[5].bone), sizeof(v.weightsUNORM[5].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[6].bone), sizeof(v.weightsUNORM[6].bone));
			bin.append(reinterpret_cast<const char*>(&v.weightsUNORM[7].bone), sizeof(v.weightsUNORM[7].bone));
		}
	}

	model.bufferViews.push_back(view);
}

bool exportCreatePrimitives(tinygltf::Model& model, QByteArray& bin, const BSMesh* bsmesh, tinygltf::Mesh& gltfMesh,
							quint32& attributeIndex, quint32 lodLevel, int materialID, GltfStore& gltf, qint32 meshLodLevel = -1)
{
	if ( lodLevel >= bsmesh->meshes.size() )
		return false;

	auto& mesh = bsmesh->meshes[lodLevel];
	auto prim = tinygltf::Primitive();

	// TODO: Full Materials, create empty Material for now
	prim.material = materialID;

	exportCreatePrimitive(model, bin, mesh, prim, "POSITION", mesh->positions.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, attributeIndex, gltf);
	exportCreatePrimitive(model, bin, mesh, prim, "NORMAL", mesh->normals.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, attributeIndex, gltf);
	exportCreatePrimitive(model, bin, mesh, prim, "TANGENT", mesh->tangentsBasis.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	if ( mesh->coords.size() > 0 ) {
		exportCreatePrimitive(model, bin, mesh, prim, "TEXCOORD_0", mesh->coords[0].size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2, attributeIndex, gltf);
	}
	if ( mesh->coords.size() > 1 ) {
		exportCreatePrimitive(model, bin, mesh, prim, "TEXCOORD_1", mesh->coords[1].size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2, attributeIndex, gltf);
	}
	exportCreatePrimitive(model, bin, mesh, prim, "COLOR_0", mesh->colors.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	
	if ( mesh->weights.size() > 0 && mesh->weightsPerVertex > 0 ) {
		exportCreatePrimitive(model, bin, mesh, prim, "WEIGHTS_0", mesh->weights.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	}

	if ( mesh->weights.size() > 0 && mesh->weightsPerVertex > 4 ) {
		exportCreatePrimitive(model, bin, mesh, prim, "WEIGHTS_1", mesh->weights.size(), TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	}

	if ( mesh->weights.size() > 0 && mesh->weightsPerVertex > 0 ) {
		Q_ASSERT(gltf.bones.size() > 0);
		exportCreatePrimitive(model, bin, mesh, prim, "JOINTS_0", mesh->weights.size(), TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	}

	if ( mesh->weights.size() > 0 && mesh->weightsPerVertex > 4 ) {
		Q_ASSERT(gltf.bones.size() > 0);
		exportCreatePrimitive(model, bin, mesh, prim, "JOINTS_1", mesh->weights.size(), TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_TYPE_VEC4, attributeIndex, gltf);
	}

	QVector<Triangle>& tris = mesh->triangles;
	if ( meshLodLevel >= 0 ) {
		tris = bsmesh->gpuLODs[meshLodLevel];
	}

	// Triangle Indices
	auto acc = tinygltf::Accessor();
	acc.bufferView = attributeIndex;
	acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
	acc.count = tris.size() * 3;
	acc.type = TINYGLTF_TYPE_SCALAR;

	prim.indices = attributeIndex++;

	model.accessors.push_back(acc);

	tinygltf::BufferView view;
	view.buffer = 0;
	view.byteOffset = bin.size();
	view.byteLength = acc.count * tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type);
	view.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

	bin.reserve(bin.size() + view.byteLength);
	for ( const auto v : tris ) {
		bin.append(reinterpret_cast<const char*>(&v[0]), sizeof(v[0]));
		bin.append(reinterpret_cast<const char*>(&v[1]), sizeof(v[1]));
		bin.append(reinterpret_cast<const char*>(&v[2]), sizeof(v[2]));
	}
	
	model.bufferViews.push_back(view);

	gltfMesh.primitives.push_back(prim);

	return true;
}

bool exportCreateMeshes(const NifModel* nif, const Scene* scene, tinygltf::Model& model, QByteArray& bin, GltfStore& gltf)
{
	int meshIndex = 0;
	quint32 attributeIndex = model.bufferViews.size();
	auto& nodes = scene->nodes.list();
	for ( const auto node : nodes ) {
		auto nodeId = node->id();
		auto iBlock = nif->getBlockIndex(nodeId);
		if ( nif->blockInherits(iBlock, "BSGeometry") ) {
			if ( gltf.nodes.value(nodeId, {}).size() == 0 )
				continue;

			auto& n = gltf.nodes[nodeId];
			auto mesh = static_cast<BSMesh*>(node);
			if ( mesh ) {
				int createdMeshes = mesh->meshCount();
				int skeletalLodIndex = -1;
				bool hasGPULODs = mesh->gpuLODs.size() > 0;
				if ( hasGPULODs )
					createdMeshes = mesh->gpuLODs.size() + 1;

				for ( int j = 0; j < createdMeshes; j++ ) {
					auto& gltfNode = model.nodes[n[j]];
					tinygltf::Mesh gltfMesh;
					gltfNode.mesh = meshIndex;
					gltfMesh.name = QString("%1%2%3").arg(node->getName()).arg(":LOD").arg(j).toStdString();
					int materialID = gltf.materials.indexOf(mesh->materialPath) + 1;
					int lodLevel = (hasGPULODs) ? 0 : j;
					if ( exportCreatePrimitives(model, bin, mesh, gltfMesh, attributeIndex, lodLevel, materialID, gltf, skeletalLodIndex) ) {
						meshIndex++;
						model.meshes.push_back(gltfMesh);
						if ( hasGPULODs ) {
							skeletalLodIndex++;
						}
					} else {
						gltf.errors << QString("%1 creation failed").arg(QString::fromStdString(gltfMesh.name));
						return false;
					}
				}
			}
		}
	}
	return true;
}


void exportGltf(const NifModel* nif, const Scene* scene, const QModelIndex& index)
{
	QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(), tr("Choose a .glTF file for export"), nif->getFilename(), "glTF (*.gltf)");
	if ( filename.isEmpty() )
		return;

	QString buffName = filename;
	buffName = QString(buffName.remove(".gltf") + ".bin");

	tinygltf::TinyGLTF writer;
	tinygltf::Model model;
	model.asset.generator = "NifSkope glTF 2.0 Exporter v1.0";

	GltfStore gltf;
	QByteArray buffer;
	bool success = exportCreateNodes(nif, scene, model, buffer, gltf);
	if ( success )
		success = exportCreateMeshes(nif, scene, model, buffer, gltf);
	if ( success ) {
		auto buff = tinygltf::Buffer();
		buff.name = buffName.toStdString();
		buff.data = std::vector<unsigned char>(buffer.cbegin(), buffer.cend());
		model.buffers.push_back(buff);

		gltf.materials.prepend("Default");
		for ( const auto& name : gltf.materials ) {
			auto mat = tinygltf::Material();
			mat.name = name.toStdString();
			model.materials.push_back(mat);
		}

		writer.WriteGltfSceneToFile(&model, filename.toStdString(), false, false, false, false);
	}

	for ( const auto& msg : gltf.errors ) {
		qCCritical(nsIo) << msg;
	}
}
