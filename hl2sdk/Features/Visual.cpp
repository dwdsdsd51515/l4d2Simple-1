﻿#include "Visual.h"
#include "../Utils/math.h"
#include "../hook.h"
#include <sstream>

CVisualPlayer* g_pVisualPlayer = nullptr;

#ifdef _DEBUG
#define Assert_Entity(_e)		(_e == nullptr || !_e->IsAlive())
#else
#define Assert_Entity(_e)		0
#endif

#define IsSurvivor(_id)				(_id == ET_SURVIVORBOT || _id == ET_CTERRORPLAYER)
#define IsCommonInfected(_id)		(_id == ET_INFECTED || _id == ET_WITCH)
#define IsSpecialInfected(_id)		(_id == ET_BOOMER || _id == ET_HUNTER || _id == ET_SMOKER || _id == ET_SPITTER || _id == ET_JOCKEY || _id == ET_CHARGER || _id == ET_TANK)

CVisualPlayer::CVisualPlayer() : CBaseFeatures::CBaseFeatures()
{
}

CVisualPlayer::~CVisualPlayer()
{
	CBaseFeatures::~CBaseFeatures();
}

void CVisualPlayer::OnEnginePaint(PaintMode_t mode)
{
	CBasePlayer* local = g_pClientPrediction->GetLocalPlayer();
	if (local == nullptr)
		return;

	// 用于格式化字符串
	std::stringstream ss;
	ss.sync_with_stdio(false);
	ss.tie(nullptr);
	ss.setf(std::ios::fixed);
	ss.precision(0);

	int team = local->GetTeam();
	int maxEntity = g_pClientInterface->EntList->GetHighestEntityIndex();

	for (int i = 1; i <= maxEntity; ++i)
	{
		CBasePlayer* entity = reinterpret_cast<CBasePlayer*>(g_pClientInterface->EntList->GetClientEntity(i));
		if (entity == nullptr || !entity->IsAlive())
			continue;

		Vector head, foot;
		auto boundingBox = entity->GetBoundingBox();
		if (!math::WorldToScreen(boundingBox.first, foot) || !math::WorldToScreen(boundingBox.second, head))
			continue;

		ss.str("");
		bool friendly = (team == entity->GetTeam());

		if (m_bBox)
			DrawBox(friendly, head, foot);
		if (m_bBone)
			DrawBone(entity, friendly);
		if (m_bHeadBox)
			DrawHeadBox(entity, head);

		if (m_bHealth)
			ss << DrawHealth(entity, ss.str().empty());
		if (m_bName)
			ss << DrawName(i, ss.str().empty());
		if (m_bCharacter)
			ss << DrawCharacter(entity, ss.str().empty());
		if (m_bWeapon)
			ss << DrawWeapon(entity, ss.str().empty());
		if (m_bDistance)
			ss << DrawDistance(entity, local, ss.str().empty());

		if (ss.str().empty())
			continue;

		if (GetTextPosition(ss.str(), head) == DP_Left)
		{
			int pixels = GetTextMaxWidth(ss.str()) * g_pDrawing->m_iFontSize;
			g_pDrawing->DrawText(head.x - pixels, head.y, (friendly ? CDrawing::SKYBLUE : CDrawing::RED),
				false, ss.str().c_str());
		}
		else
		{
			g_pDrawing->DrawText(head.x, head.y, (friendly ? CDrawing::SKYBLUE : CDrawing::RED),
				false, ss.str().c_str());
		}
	}
}

void CVisualPlayer::OnSceneEnd()
{
	if (!m_bChams)
		return;

	static IMaterial* chamsMaterial = nullptr;
	if (chamsMaterial == nullptr)
	{
		if (g_pClientHook->oFindMaterial != nullptr)
		{
			chamsMaterial = g_pClientHook->oFindMaterial(g_pClientInterface->MaterialSystem,
				XorStr("debug/debugambientcube"), XorStr("Model textures"), true, nullptr);
		}
		else
		{
			chamsMaterial = g_pClientInterface->MaterialSystem->FindMaterial(
				XorStr("debug/debugambientcube"), XorStr("Model textures"));
		}

		chamsMaterial->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
		chamsMaterial->ColorModulate(1.0f, 1.0f, 1.0f);
	}

	CBasePlayer* local = g_pClientPrediction->GetLocalPlayer();
	if (local == nullptr || chamsMaterial == nullptr)
		return;

	int team = local->GetTeam();
	int maxEntity = g_pClientInterface->EntList->GetHighestEntityIndex();

	for (int i = 1; i <= maxEntity; ++i)
	{
		CBasePlayer* entity = reinterpret_cast<CBasePlayer*>(g_pClientInterface->EntList->GetClientEntity(i));
		if (entity == nullptr || !entity->IsAlive())
			continue;

		if(team == entity->GetTeam())
			chamsMaterial->ColorModulate(0.0f, 1.0f, 1.0f);
		else
			chamsMaterial->ColorModulate(1.0f, 0.0f, 0.0f);

		chamsMaterial->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);
		g_pClientInterface->ModelRender->ForcedMaterialOverride(chamsMaterial);
		entity->DrawModel(0x1);
		g_pClientInterface->ModelRender->ForcedMaterialOverride(nullptr);
	}
}

void CVisualPlayer::OnMenuDrawing()
{
	if (!ImGui::TreeNode(XorStr("VisualPlayer")))
		return;

	ImGui::Checkbox(XorStr("Left alignment"), &m_bDrawToLeft);
	ImGui::Checkbox(XorStr("Player Box"), &m_bBox);
	ImGui::Checkbox(XorStr("Player Head"), &m_bHeadBox);
	ImGui::Checkbox(XorStr("Player Bone"), &m_bBone);
	ImGui::Checkbox(XorStr("Player Name"), &m_bName);
	ImGui::Checkbox(XorStr("Player Health"), &m_bHealth);
	ImGui::Checkbox(XorStr("Player Distance"), &m_bDistance);
	ImGui::Checkbox(XorStr("Player Weapon"), &m_bWeapon);
	ImGui::Checkbox(XorStr("Player Character"), &m_bCharacter);
	ImGui::Checkbox(XorStr("Player Chams"), &m_bChams);

	ImGui::TreePop();
}

bool CVisualPlayer::HasTargetVisible(CBasePlayer * entity)
{
	CBasePlayer* local = g_pClientPrediction->GetLocalPlayer();
	if (local == nullptr)
		return false;

	Ray_t ray;
	CTraceFilter filter;
	ray.Init(local->GetEyePosition(), entity->GetHeadOrigin());
	filter.pSkip1 = local;

	trace_t trace;

	try
	{
		g_pClientInterface->Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);
	}
	catch (...)
	{
		Utils::log(XorStr("CKnifeBot.HasEnemyVisible.TraceRay Error."));
		return false;
	}

	return (trace.m_pEnt == entity || trace.fraction > 0.97f);
}

void CVisualPlayer::DrawBox(bool friendly, const Vector & head, const Vector & foot)
{
	if (friendly)
		g_pDrawing->DrawCorner(head.x, head.y, abs(foot.x - head.x), abs(foot.y - head.y), CDrawing::SKYBLUE);
	else
		g_pDrawing->DrawCorner(head.x, head.y, abs(foot.x - head.x), abs(foot.y - head.y), CDrawing::RED);
}

void CVisualPlayer::DrawBone(CBasePlayer * entity, bool friendly)
{
	model_t* models = entity->GetModel();
	if (models == nullptr)
		return;

	studiohdr_t* hdr = g_pClientInterface->ModelInfo->GetStudiomodel(models);
	if (hdr == nullptr)
		return;

	Vector parent, child, screenParent, screenChild;
	D3DCOLOR color = (friendly ? CDrawing::SKYBLUE : CDrawing::RED);

	for (int i = 0; i < hdr->numbones; ++i)
	{
		mstudiobone_t* bone = hdr->pBone(i);
		if (bone == nullptr || !(bone->flags & 0x100) || bone->parent == -1)
			continue;

		child = entity->GetBoneOrigin(i);
		parent = entity->GetBoneOrigin(bone->parent);
		if (!child.IsValid() || !parent.IsValid() ||
			!math::WorldToScreen(parent, screenParent) ||
			!math::WorldToScreen(child, screenChild))
			continue;

		g_pDrawing->DrawLine(screenParent.x, screenParent.y, screenChild.x, screenChild.y, color);
	}
}

void CVisualPlayer::DrawHeadBox(CBasePlayer* entity, const Vector & head)
{
	int classId = entity->GetClassID();
	D3DCOLOR color = CDrawing::WHITE;
	bool visible = HasTargetVisible(entity);

	if (IsSurvivor(classId))
		color = CDrawing::SKYBLUE;
	else if (IsSpecialInfected(classId))
		color = CDrawing::RED;
	else if (classId == ET_WITCH)
		color = CDrawing::PINK;
	else if (classId == ET_INFECTED)
		color = CDrawing::ORANGE;

	if (visible)
		g_pDrawing->DrawCircleFilled(head.x, head.y, 3, color, 8);
	else
		g_pDrawing->DrawCircle(head.x, head.y, 3, color, 8);
}

int CVisualPlayer::GetTextMaxWidth(const std::string & text)
{
	size_t width = 0;
	for (auto line : Utils::Split(text, "\n"))
	{
		if (line.length() > width)
			width = line.length();
	}

	return static_cast<int>(width);
}

typename CVisualPlayer::DrawPosition_t CVisualPlayer::GetTextPosition(const std::string & text, const Vector & head)
{
	int width = 0, height = 0;
	g_pClientInterface->Engine->GetScreenSize(width, height);

	int pixels = GetTextMaxWidth(text) * g_pDrawing->m_iFontSize;
	if (m_bDrawToLeft)
	{
		if ((head.x - pixels) < 0)
			return DP_Right;

		return DP_Left;
	}

	if ((head.x + pixels) > width)
		return DP_Left;

	return DP_Right;
}

std::string CVisualPlayer::DrawName(int index, bool separator)
{
	player_info_t info;
	if (!g_pClientInterface->Engine->GetPlayerInfo(index, &info))
		return "";

	if (separator)
		return std::string("\n") + info.name;

	return info.name;
}

std::string CVisualPlayer::DrawHealth(CBasePlayer * entity, bool separator)
{
	char buffer[32];

	if (entity->GetTeam() == 3)
		sprintf_s(buffer, "[%d] ", entity->GetHealth());
	else
		sprintf_s(buffer, "[%d] ", entity->GetHealth() + entity->GetTempHealth());
	
	if (separator)
		return std::string("\n") + buffer;

	return buffer;
}

std::string CVisualPlayer::DrawWeapon(CBasePlayer * entity, bool separator)
{
	if (entity->GetTeam() != 2)
		return "";

	CBaseWeapon* weapon = entity->GetActiveWeapon();
	if (weapon == nullptr)
		return "";

	char buffer[64];
	const char* weaponName = weapon->GetWeaponName();
	if (weapon->IsReloading())
		sprintf_s(buffer, "%s %s", weaponName, XorStr("(reloading)"));
	else
		sprintf_s(buffer, "%s (%d/%d)", weaponName, weapon->GetClip(), weapon->GetAmmo());

	if (separator)
		return std::string("\n") + buffer;

	return buffer;
}

std::string CVisualPlayer::DrawDistance(CBasePlayer * entity, CBasePlayer * local, bool separator)
{
	char buffer[16];
	_itoa_s(static_cast<int>(math::GetVectorDistance(entity->GetAbsOrigin(), local->GetAbsOrigin(), true)),
		buffer, 10);

	if (separator)
		return std::string("\n") + buffer;

	return buffer;
}

std::string CVisualPlayer::DrawCharacter(CBasePlayer * entity, bool separator)
{
	int classId = entity->GetClassID();
	std::string buffer;

	switch (classId)
	{
	case ET_SMOKER:
		buffer = XorStr("Smoker");
		break;
	case ET_BOOMER:
		buffer = XorStr("Boomer");
		break;
	case ET_HUNTER:
		buffer = XorStr("Hunter");
		break;
	case ET_SPITTER:
		buffer = XorStr("Spitter");
		break;
	case ET_JOCKEY:
		buffer = XorStr("Jockey");
		break;
	case ET_CHARGER:
		buffer = XorStr("Charger");
		break;
	case ET_WITCH:
		buffer = XorStr("Witch");
		break;
	case ET_TANK:
		buffer = XorStr("Tank");
		break;
	case ET_SURVIVORBOT:
	case ET_CTERRORPLAYER:
		const char* models = entity->GetNetProp<const char*>(XorStr("DT_BasePlayer"), XorStr("m_ModelName"));
		if (_stricmp(models, XorStr("models/survivors/survivor_gambler.mdl")))
			buffer = XorStr("Nick");
		else if (_stricmp(models, XorStr("models/survivors/survivor_producer.mdl")))
			buffer = XorStr("Rochelle");
		else if (_stricmp(models, XorStr("models/survivors/survivor_coach.mdl")))
			buffer = XorStr("Coach");
		else if (_stricmp(models, XorStr("models/survivors/survivor_mechanic.mdl")))
			buffer = XorStr("Ellis");
		else if (_stricmp(models, XorStr("models/survivors/survivor_namvet.mdl")))
			buffer = XorStr("Bill");
		else if (_stricmp(models, XorStr("models/survivors/survivor_namvet.mdl")))
			buffer = XorStr("Zoey");
		else if (_stricmp(models, XorStr("models/survivors/survivor_biker.mdl")))
			buffer = XorStr("Francis");
		else if (_stricmp(models, XorStr("models/survivors/survivor_manager.mdl")))
			buffer = XorStr("Louis");
		break;
	}

	if (!buffer.empty())
	{
		if (separator)
			buffer = "\n" + buffer;
	}

	return "";
}