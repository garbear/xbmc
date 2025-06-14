/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "games/agents/input/AgentInputMap.h"
#include "games/agents/input/AgentInputMapXML.h"
#include "games/agents/input/AgentTopology.h"
#include "games/agents/input/AgentTopologyXML.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <gtest/gtest.h>

// These unit tests validate XML serialization of agent input maps and the
// error handling for malformed documents.

using namespace KODI;
using namespace GAME;

namespace
{
/*
 * Create a small topology for unit tests.
 *
 * A single controller port is populated with one compatible controller.
 * The ID, game client and digest are set so the topology behaves like a
 * real-world definition.
 */
std::shared_ptr<CAgentTopology> BuildSampleTopology(unsigned int id)
{
  // Build a controller instance using the default controller's ID
  ADDON::AddonInfoPtr addonInfo =
      std::make_shared<ADDON::CAddonInfo>(DEFAULT_CONTROLLER_ID, ADDON::AddonType::GAME_CONTROLLER);
  ControllerPtr controller = std::make_shared<CController>(addonInfo);

  // Build a single controller port tree
  CControllerNode node;
  node.SetController(controller);

  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node});

  CControllerTree hub;
  hub.SetPorts({port});

  // Serialize the hub to XML so it can be reloaded
  CXBMCTinyXML2 doc;

  // Use <logicaltopology> as the root element to match game add-on topology.xml
  auto* root = doc.NewElement("logicaltopology");
  EXPECT_TRUE(hub.Serialize(*root));
  doc.InsertFirstChild(root);

  // Populate the topology object from the controller tree
  auto topology = std::make_shared<CAgentTopology>();
  EXPECT_TRUE(topology->DeserializeControllerTree(doc));
  topology->SetID(id);
  topology->AddGameClient("game.client.test");

  // Compute a digest to emulate real metadata
  topology->UpdateDigest();

  return topology;
}
} // namespace

/*!
 * Verify topology serialization and deserialization preserves all data.
 *
 * A sample topology is converted to XML and parsed back. Every property of
 * the resulting topology should match the original object.
 */
TEST(TestAgentInputMapXML, SerializeDeserializeTopology)
{
  auto topology = BuildSampleTopology(1);

  // Serialize the topology
  CXBMCTinyXML2 doc1;
  auto* root1 = doc1.NewElement("logicaltopology");
  ASSERT_TRUE(CAgentTopologyXML::SerializeTopology(*root1, *topology));
  doc1.InsertFirstChild(root1);
  tinyxml2::XMLPrinter printer1;
  doc1.Print(&printer1);
  std::string xml = printer1.CStr();

  // Parse the XML string back into a topology
  CXBMCTinyXML2 doc2;
  ASSERT_TRUE(doc2.Parse(xml));
  const tinyxml2::XMLElement* root2 = doc2.RootElement();
  ASSERT_NE(root2, nullptr);

  CAgentTopology topology2;
  ASSERT_TRUE(CAgentTopologyXML::DeserializeTopology(*root2, topology2));

  // Verify the deserialized topology
  EXPECT_EQ(topology2.GetID(), topology->GetID());
  EXPECT_EQ(topology2.GetDigest(), topology->GetDigest());
  EXPECT_EQ(topology2.GetDigestCreationUTC(), topology->GetDigestCreationUTC());
  EXPECT_EQ(topology2.GetGameClients(), topology->GetGameClients());

  ASSERT_EQ(topology2.GetControllerTree().GetPorts().size(), 1u);
  const CPortNode& port = topology2.GetControllerTree().GetPorts().front();
  EXPECT_EQ(port.GetPortType(), PORT_TYPE::CONTROLLER);
  EXPECT_EQ(port.GetPortID(), "1");
}

/*!
 * Verify round-tripping a map of topologies through XML.
 *
 * The map should contain the same entries after deserialization and allow
 * lookup by both ID and digest.
 */
TEST(TestAgentInputMapXML, SerializeDeserializeMap)
{
  auto topology = BuildSampleTopology(2);

  // Map containing the single topology keyed by ID
  CAgentInputMap::TopologyIDMap map;
  map[topology->GetID()] = topology;

  CXBMCTinyXML2 doc1;
  ASSERT_TRUE(CAgentInputMapXML::SerializeTopologies(doc1, map));
  tinyxml2::XMLPrinter printer1;
  doc1.Print(&printer1);
  std::string xml = printer1.CStr();

  // Deserialize the XML and verify both lookup maps
  CXBMCTinyXML2 doc2;
  ASSERT_TRUE(doc2.Parse(xml));

  CAgentInputMap::TopologyIDMap map2;
  CAgentInputMap::TopologyDigestMap map2Digest;
  ASSERT_TRUE(CAgentInputMapXML::DeserializeTopologies(doc2, map2, map2Digest));

  ASSERT_EQ(map2.size(), 1u);
  auto it = map2.find(topology->GetID());
  ASSERT_NE(it, map2.end());
  EXPECT_EQ(it->second->GetDigest(), topology->GetDigest());

  ASSERT_EQ(map2Digest.size(), 1u);
  auto itDigest = map2Digest.find(topology->GetDigest());
  ASSERT_NE(itDigest, map2Digest.end());
  EXPECT_EQ(itDigest->second->GetID(), topology->GetID());
}

/*!
 * Fail deserialization when the root element name isn't <topologies>.
 *
 * Parsing should fail and return false.
 */
TEST(TestAgentInputMapXML, DeserializeTopologiesInvalidRoot)
{
  // Root name does not match <topologies>
  const char* xml = "<invalid/>";
  std::string xmlStr{xml};

  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));

  CAgentInputMap::TopologyIDMap map;
  CAgentInputMap::TopologyDigestMap mapDigest;

  EXPECT_FALSE(CAgentInputMapXML::DeserializeTopologies(doc, map, mapDigest));
}

/*!
 * Fail deserialization when <logicaltopology> elements are missing.
 *
 * Without a topology entry the call should return false.
 */
TEST(TestAgentInputMapXML, DeserializeTopologiesMissingTopology)
{
  // No <logicaltopology> entries present
  const char* xml = "<topologies/>";
  std::string xmlStr{xml};

  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));

  CAgentInputMap::TopologyIDMap map;
  CAgentInputMap::TopologyDigestMap mapDigest;

  EXPECT_FALSE(CAgentInputMapXML::DeserializeTopologies(doc, map, mapDigest));
}

/*!
 * Fail deserialization of a topology when the <definition> child is absent.
 */
TEST(TestAgentInputMapXML, DeserializeTopologyMissingDefinition)
{
  // Remove the required <definition> element before parsing
  auto topology = BuildSampleTopology(3);

  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("logicaltopology");
  ASSERT_TRUE(CAgentTopologyXML::SerializeTopology(*root, *topology));
  doc.InsertFirstChild(root);

  tinyxml2::XMLElement* def = root->FirstChildElement("definition");
  ASSERT_NE(def, nullptr);
  root->DeleteChild(def);

  CAgentTopology result;
  EXPECT_FALSE(CAgentTopologyXML::DeserializeTopology(*root, result));
}

/*!
 * Fail deserialization of a topology missing the <gameclients> child.
 */
TEST(TestAgentInputMapXML, DeserializeTopologyMissingGameClients)
{
  // Remove the <gameclients> element before parsing
  auto topology = BuildSampleTopology(4);

  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("logicaltopology");
  ASSERT_TRUE(CAgentTopologyXML::SerializeTopology(*root, *topology));
  doc.InsertFirstChild(root);

  tinyxml2::XMLElement* gameClients = root->FirstChildElement("gameclients");
  ASSERT_NE(gameClients, nullptr);
  root->DeleteChild(gameClients);

  CAgentTopology result;
  EXPECT_FALSE(CAgentTopologyXML::DeserializeTopology(*root, result));
}
