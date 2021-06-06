#include <algorithm>
#include <assert.h>
#include "Utils.h"

#include "Mario.h"
#include "Game.h"

#include "Goomba.h"
#include "Portal.h"
#include "FireBall.h"
#include "PiranhaPlant.h"
#include "Brick.h"
#include "PipeHitBox.h"
#include "ColorBrickHitBox.h"
#include "QBrick.h"
#include "Ground.h"

#include "Map.h"

CMario::CMario(float x, float y) : CGameObject()
{
	level = MARIO_LEVEL_SMALL;
	untouchable = 0;
	transforming = 0;
	SetState(MARIO_STATE_IDLE);

	start_x = x;
	start_y = y;
	this->x = x;
	this->y = y;
}

void CMario::Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects)
{
	if (y > Map::getInstance()->getHeight()) {
		Reset();
	}

	if (transforming && GetTickCount() - startTransforming < 800) {
		return;
	}
	if (isFlying && GetTickCount() - startFlying > 200) {
		if (GetTickCount() - startFlying > 2000) {
			SetState(MARIO_STATE_FALL);
		}
		else {
			SetState(MARIO_STATE_GLIDE);
		}
	}

	//for (UINT i = 0; i < coObjects->size(); ++i) {
	//	float x, y;
	//	coObjects->at(i)->GetPosition(x, y);
	//	//DebugOut(L"[X-Y] %f - %f\n", x, y);
	//	if (x == 353.0f && y == 384.0f) {
	//		DebugOut(L"abc\n");
	//	}
	//}

	/*for (UINT i = 0; i < coObjects->size(); ++i) {
		float x, y;
		coObjects->at(i)->GetPosition(x, y);
		DebugOut(L"[OBJ] %f - %f\n", x, y);
		if ((x == 480.0f || x == 496.0f) && y == 416.0f) {
			DebugOut(L"abc\n");
		}
	}*/

	oldX = x;
	oldY = y;

	/*if (abs(vx) < MARIO_WALKING_MAX_SPEED && (state == MARIO_STATE_WALKING_RIGHT || state == MARIO_STATE_WALKING_RIGHT)) {
		vx += accelerationX * dt;
	}*/

	/*if (isFlying) {
		vy = (flyUp) ? MARIO_RACOON_FLY_VY : MARIO_RACOON_FALL_VY;
		flyUp = !flyUp;
	}*/

	// Calculate dx, dy 
	CGameObject::Update(dt);

	float _dx = dx, _dy = dy;

	Grid::GetInstance()->clearObjFromGrid(this);

	// Simple fall down
	if(!isFlying && !isFalling && !isGliding) vy += MARIO_GRAVITY * dt;

	vector<LPCOLLISIONEVENT> coEvents;
	vector<LPCOLLISIONEVENT> coEventsResult;
	vector<UINT> exceptions;

	float l1, r1, t1, b1;
	float l2, r2, t2, b2;

	GetBoundingBox(l1, t1, r1, b1);

	coEvents.clear();

	/*for (UINT i = 0; i < coObjects->size(); ++i) {
		if (dynamic_cast<ColorBrickHitBox*>(coObjects->at(i))) {
			coObjects->at(i)->GetBoundingBox(l2, t2, r2, b2);
			if (doOverlap(l1, t1, r1, b1, l2, t2, r2, b2)) {
				exceptions.push_back(i);
			}
		}
	}

	for (auto& ind : exceptions) {
		remove(coObjects->begin(), coObjects->end(), coObjects->at(ind));
	}*/

	// turn off collision when die 

	/*if (GetTickCount() - untouchable_start <= MARIO_UNTOUCHABLE_TIME) {
		coObjects->erase(std::remove_if(coObjects->begin(), coObjects->end(),
			[](auto& obj) { return dynamic_cast<FireBall*>(obj); }), coObjects->end());
	}*/

	if (state != MARIO_STATE_DIE)
		CalcPotentialCollisions(coObjects, coEvents);

	// reset untouchable timer if untouchable time has passed
	if (GetTickCount() - untouchable_start > MARIO_UNTOUCHABLE_TIME)
	{
		untouchable_start = 0;
		untouchable = 0;
	}

	// No collision occured, proceed normally
	if (coEvents.size() == 0)
	{
		x += dx;
		y += dy;

		/*Grid::GetInstance()->putObjectIntoGrid(this);
		for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];

		return;*/
	}
	else
	{
		float min_tx, min_ty, nx = 0, ny;
		float rdx = 0;
		float rdy = 0;

		// TODO: This is a very ugly designed function!!!!
		FilterCollision(coEvents, coEventsResult, min_tx, min_ty, nx, ny, rdx, rdy);

		// how to push back Mario if collides with a moving objects, what if Mario is pushed this way into another object?
		//if (rdx != 0 && rdx!=dx)
		//	x += nx*abs(rdx); 

		// block every object first!
		x += min_tx * dx + nx * 0.4f;
		y += min_ty * dy + ny * 0.4f;

		float temp = vy;
		//if (nx != 0) vx = 0;
		//if (ny != 0) vy = 0;

		//
		// Collision logic with other objects
		//
		float px, py;

		for (UINT i = 0; i < coEventsResult.size(); i++)
		{
			LPCOLLISIONEVENT e = coEventsResult[i];

			isStanding = (e->ny < 0) ? e->obj->GetCanBeStandOn() : false;
			if (isStanding) {
				isJumping = 0;
				isFlying = 0;
				isFalling = 0;
				isGliding = 0;
				isFallingTail = 0;
			}
			//isJumping = isFlying = isFalling = !isStanding;

			if (dynamic_cast<CGoomba*>(e->obj)) // if e->obj is Goomba 
			{
				CGoomba* goomba = dynamic_cast<CGoomba*>(e->obj);

				// jump on top >> kill Goomba and deflect a bit 
				if (e->ny < 0)
				{
					if (goomba->GetState() != GOOMBA_STATE_DIE)
					{
						goomba->SetState(GOOMBA_STATE_DIE);
						vy = -MARIO_JUMP_DEFLECT_SPEED;
					}
				}
				else if (e->nx != 0)
				{
					if (untouchable == 0)
					{
						if (goomba->GetState() != GOOMBA_STATE_DIE)
						{
							if (level > MARIO_LEVEL_SMALL)
							{
								//level = MARIO_LEVEL_SMALL;
								backupLevel = MARIO_LEVEL_SMALL;
								backupState = state;
								startTransforming = GetTickCount();
								turnIntoSmall();
								StartUntouchable();
							}
							else
								SetState(MARIO_STATE_DIE);
						}
					}
				}
			} // if Goomba
			else if (dynamic_cast<CPortal*>(e->obj))
			{
				CPortal* p = dynamic_cast<CPortal*>(e->obj);
				CGame::GetInstance()->SwitchScene(p->GetSceneId());
			}
			else if (dynamic_cast<FireBall*>(e->obj)) {
				x -= min_tx * dx + nx * 0.4f;
				y -= min_ty * dy + ny * 0.4f;

				string s;
				float px, py;

				//for (auto& ev : coEventsResult) {
				//	/*std::wstring stemp = std::wstring(s.begin(), s.end());
				//	LPCWSTR sw = stemp.c_str();*/

				//	e->obj->GetPosition(px, py);

				//	DebugOut(L"[COS] %f %f\n", px, py);
				//}

				/*remove(coObjects->begin(), coObjects->end(), e->obj);

				Update(dt, coObjects);*/

				//if (untouchable == 0)
				//{
				//	if (level > MARIO_LEVEL_SMALL)
				//	{
				//		//level = MARIO_LEVEL_SMALL;
				//		turnIntoSmall();
				//		StartUntouchable();
				//	}
				//	else
				//		SetState(MARIO_STATE_DIE);
				//}
			}
			else if (dynamic_cast<PiranhaPlant*>(e->obj)) {
				x -= min_tx * dx + nx * 0.4f;
				y -= min_ty * dy + ny * 0.4f;

				if (!untouchable)
				{
					if (level == MARIO_LEVEL_BIG)
					{
						//level = MARIO_LEVEL_SMALL;
						backupLevel = MARIO_LEVEL_SMALL;
						backupState = state;
						startTransforming = GetTickCount();
						turnIntoSmall();
						StartUntouchable();
					}
					else if (level == MARIO_LEVEL_RACOON) {

					}
					else
						SetState(MARIO_STATE_DIE);
				}
				else {
					x += _dx;
					y += _dy;
				}
			}
			else if (dynamic_cast<Ground*>(e->obj) || dynamic_cast<PipeHitBox*>(e->obj)) {
				/*x += min_tx * dx + nx * 0.4f;
				y += min_ty * dy + ny * 0.4f;*/


				_dx = _dy = 0;

				if (e->nx != 0) {
					vx = 0;
					isRunning = 0;
					isSliding = 0;
				}
				else if (e->ny) {
					vy = 0;
				}
				/*Grid::GetInstance()->putObjectIntoGrid(this);
				for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];

				return;*/
			}
			else if (dynamic_cast<QBrick*>(e->obj)) {
				/*x += min_tx * dx + nx * 0.4f;
				y += min_ty * dy + ny * 0.4f;*/

				if (e->nx != 0) {
					vx = 0;
					isRunning = 0;
					isSliding = 0;
				}
				else if (e->ny != 0) vy = 0;

				if (e->ny > 0) {
					QBrick* qBrick = dynamic_cast<QBrick*>(e->obj);

					qBrick->PopUpHiddenItem();
				}
			}
			else if (dynamic_cast<ColorBrickHitBox*>(e->obj)) {
				/*x += min_tx * dx + nx * 0.4f;
				y += min_ty * dy + ny * 0.4f;*/

				//if (e->ny < 0) {
				//	//x -= min_tx * dx + nx * 0.4f;
				//	//y -= min_ty * dy + ny * 0.4f;



				//	//y += vy * e->t;
				//	vy = 0;
				//}
				//else {
				//	//vy = temp;
				//	x += _dx;
				//	y += _dy;
				//}

				if (e->ny < 0) {
					vy = 0;
				}
				else {
					x -= min_tx * dx + nx * 0.4f;
					y -= min_ty * dy + ny * 0.4f;

					x += _dx;
					y += _dy;
				}
			}
			else if (dynamic_cast<Mushroom*>(e->obj)) {
				x -= min_tx * dx + nx * 0.4f;
				y -= min_ty * dy + ny * 0.4f;

				/*backupLevel = MARIO_LEVEL_BIG;
				backupState = state;
				startTransforming = GetTickCount();
				turnIntoBig();*/
				/*Mushroom* mushroom = dynamic_cast<Mushroom*>(e->obj);

				if (e->nx || e->ny)	mushroom->GotObserved(this);*/
			}
		}
	}

	/*x += _dx;
	y += _dy;*/

	Grid::GetInstance()->putObjectIntoGrid(this);

	/*if (state != MARIO_STATE_DIE) {
		Grid::GetInstance()->putObjectIntoGrid(this);
	}*/

	// clean up collision events
	for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];
}

void CMario::Render()
{
	//DebugOut(L"[is sliding] %d - %d - %d - %d\n", isSliding, isFlying, isFalling, isJumping);

	int ani = -1;
	if (state == MARIO_STATE_DIE)
		ani = MARIO_ANI_DIE;
	else {
		if (isFalling) {
			ani = (nx > 0) ? MARIO_ANI_RACOON_FALL_RIGHT : MARIO_ANI_RACOON_FALL_LEFT;
		}
		else if (isFlying) {
			if (isGliding) {
				ani = (nx > 0) ? MARIO_ANI_RACOON_GLIDE_RIGHT : MARIO_ANI_RACOON_GLIDE_LEFT;
			}
			else {
				ani = (nx > 0) ? MARIO_ANI_RACOON_FLY_RIGHT : MARIO_ANI_RACOON_FLY_LEFT;
			}
			//SetState(MARIO_STATE_FALL);
		}
		else if (isFallingTail) {
			ani = (nx > 0) ? MARIO_ANI_RACOON_FALL_TAIL_RIGHT : MARIO_ANI_RACOON_FALL_TAIL_LEFT;
		}
		else {
			ani = filterSomeCommonAniByLevel();
		}
	}


	int alpha = 255;

	if (untouchable) {
		alpha = 128;
	}

	//DebugOut(L"[] %d\n", ani);

	animation_set->at(ani)->Render(x, y, alpha);

	RenderBoundingBox();
}

void CMario::SetState(int state)
{
	switch (state)
	{
	case MARIO_STATE_WALKING_RIGHT: {
		/*if (this->state != MARIO_STATE_WALKING_RIGHT) {
			vx = MARIO_WALKING_SPEED;
			accelerationX = MARIO_WALKING_ACCELERATION;
			nx = 1;
		}*/
		if (isRunning == 0)
			vx = MARIO_WALKING_SPEED;
		else
		{
			if (vx < MARIO_WALKING_SPEED)
				vx = MARIO_WALKING_SPEED;
			if (vx < MARIO_RUNNING_SPEED && isStanding)
				vx += MARIO_WALKING_ACCELERATION;
		}
		nx = 1;
		break;
		//if (nx < 0) {
		//	if (isFlying) {
		//		vx *= -1;
		//		vy = MARIO_RACOON_FLY_VY;
		//	}
		//}
		//else if (isRunning && isStanding) {
		//	if (vx < MARIO_RUNNING_SPEED) {
		//		vx += MARIO_WALKING_ACCELERATION;
		//	}
		//	else if (vx < MARIO_WALKING_SPEED) {
		//		vx = MARIO_WALKING_SPEED;
		//	}
		//}
		//else {
		//	vx = MARIO_WALKING_SPEED;
		//}
		///*vx = MARIO_WALKING_SPEED;
		//accelerationX = MARIO_WALKING_ACCELERATION;*/
		//nx = 1;
		//break;
	}
	case MARIO_STATE_WALKING_LEFT: {
		if (isRunning == 0)
			vx = -MARIO_WALKING_SPEED;
		else
		{
			if (vx > -MARIO_WALKING_SPEED)
				vx = -MARIO_WALKING_SPEED;
			if (vx > -MARIO_RUNNING_SPEED && isStanding)
				vx -= MARIO_WALKING_ACCELERATION;
		}
		nx = -1;
		break;
		//if (nx > 0) {
		//	if (isFlying) {
		//		vx *= -1;
		//		vy = MARIO_RACOON_FLY_VY;
		//	}
		//}
		//else if (isRunning && isStanding) {
		//	if (vx > -MARIO_RUNNING_SPEED) {
		//		vx -= MARIO_WALKING_ACCELERATION;
		//	}
		//	else if (vx > -MARIO_WALKING_SPEED) {
		//		vx = -MARIO_WALKING_SPEED;
		//	}
		//}
		//else {
		//	vx = -MARIO_WALKING_SPEED;
		//}
		///*vx = -MARIO_WALKING_SPEED;
		//accelerationX = -MARIO_WALKING_ACCELERATION;*/
		//nx = -1;
		//break;
	}
	case MARIO_STATE_JUMP: {
		if (!isStanding) {
			return;
		}

		vy = -MARIO_JUMP_SPEED_Y;
		isStanding = false;
		isJumping = true;
		break;
	}
	case MARIO_STATE_IDLE: {
		vx = 0;
		break;
	}
	case MARIO_STATE_DIE: {
		vy = -MARIO_DIE_DEFLECT_SPEED;
		vx = 0;
		break;
	}
	case MARIO_STATE_FLY:
		/*isFlying = 1;
		isFalling = 0;
		isStanding = 0;*/
		StartFlying();
		break;
	case MARIO_STATE_FALL:
		//isFlying = 0;
		StartFalling();
		break;
	case MARIO_STATE_GLIDE:
		StartGliding();
		break;
	case MARIO_STATE_FALL_TAIL:
		vy = MARIO_RACOON_GLIDE_VY;
		isFallingTail = 1;
		break;
	}
	

	CGameObject::SetState(state);
}

int CMario::GetLevel()
{
	return level;
}

void CMario::GetBoundingBox(float& left, float& top, float& right, float& bottom)
{
	left = x;
	top = y;

	if (level == MARIO_LEVEL_BIG)
	{
		right = x + MARIO_BIG_BBOX_WIDTH;
		bottom = y + MARIO_BIG_BBOX_HEIGHT;
	}
	else if (level == MARIO_LEVEL_SMALL)
	{
		right = x + MARIO_SMALL_BBOX_WIDTH;
		bottom = y + MARIO_SMALL_BBOX_HEIGHT;
	}
	else if (level == MARIO_LEVEL_RACOON) {
		if (nx == 1) {
			left += 7;
		}
		
		right = left + MARIO_RACOON_BBOX_WIDTH;
		bottom = y + MARIO_RACOON_BBOX_HEIGHT;
	}
}

void CMario::ToBig()
{
	y -= (MARIO_BIG_BBOX_HEIGHT - MARIO_SMALL_BBOX_HEIGHT);
}

void CMario::ToSmall() {
	y += (MARIO_BIG_BBOX_HEIGHT - MARIO_SMALL_BBOX_HEIGHT);
}

void CMario::ToRacoon()
{
	y -= (MARIO_RACOON_BBOX_HEIGHT - MARIO_SMALL_BBOX_HEIGHT);
}

void CMario::finishSizeTransforming()
{
	if (transforming && GetTickCount() - startTransforming >= 800) {
		//player->SetState(player->Get)
		SetState(backupState);

		if (backupLevel == MARIO_LEVEL_BIG && level == MARIO_LEVEL_SMALL) {
			ToBig();
		}
		else if (backupLevel == MARIO_LEVEL_SMALL && level == MARIO_LEVEL_BIG) {
			ToSmall();
		}

		level = backupLevel;

		transforming = 0;
		//player->SetStartTransforming((DWORD)0);
	}
}

int CMario::filterSomeCommonAniByLevel()
{
	int ani;

	//DebugOut(L"[IS JUMPING] %d\n", isJumping);

	if (isJumping) {
		ani = (nx > 0) ? animationsByLevel[level][MARIO_ANI_JUMP_RIGHT] : animationsByLevel[level][MARIO_ANI_JUMP_LEFT];
	}
	else if (!vx && state == MARIO_STATE_IDLE) {
		ani = (nx > 0) ? animationsByLevel[level][MARIO_ANI_IDLE_RIGHT] : animationsByLevel[level][MARIO_ANI_IDLE_LEFT];
	}
	else if (!vx && state != MARIO_STATE_IDLE) {
		ani = (nx > 0) ? animationsByLevel[level][MARIO_ANI_WALKING_RIGHT] : animationsByLevel[level][MARIO_ANI_WALKING_LEFT];
	}
	else if (vx > 0) {
		if (isJumping && !isFlying) {
			ani = animationsByLevel[level][MARIO_ANI_JUMP_RIGHT];
		}
		else if (isRunning) {
			if (isSliding) {
				ani = animationsByLevel[level][MARIO_ANI_SLIDE_RIGHT];
			}
			else {
				ani = animationsByLevel[level][MARIO_ANI_RUN_RIGHT];
			}
		}
		else {
			ani = animationsByLevel[level][MARIO_ANI_WALKING_RIGHT];
		}
	}
	else {
		if (isJumping && !isFlying) {
			ani = animationsByLevel[level][MARIO_ANI_JUMP_LEFT];
		}
		else if (isRunning) {
			if (isSliding) {
				ani = animationsByLevel[level][MARIO_ANI_SLIDE_LEFT];
			}
			else {
				ani = animationsByLevel[level][MARIO_ANI_RUN_LEFT];
			}
		}
		else {
			ani = animationsByLevel[level][MARIO_ANI_WALKING_LEFT];
		}
	}

	return ani;
}

DWORD CMario::GetUntouchableStart()
{
	return untouchable_start;
}

bool CMario::GetUntouchable()
{
	return untouchable;
}

int CMario::GetBackupLevel()
{
	return backupLevel;
}

void CMario::SetBackupLevel(int val) {
	backupLevel = val;
}

int CMario::GetBackupState()
{
	return backupState;
}

void CMario::SetBackupState(int state) {
	backupState = state;
}

bool CMario::GetIsRunning()
{
	return isRunning;
}

void CMario::SetIsRunning(bool val) {
	isRunning = val;
}

bool CMario::GetIsSliding() {
	return isSliding;
}

void CMario::SetIsSliding(bool val)
{
	isSliding = val;
}

void CMario::SetIsStanding(bool val)
{
	isStanding = val;
}

bool CMario::GetIsStanding() {
	return isStanding;
}

void CMario::SetIsJumping(bool val)
{
	isJumping = val;
}

bool CMario::GetIsJumping() {
	return isJumping;
}

void CMario::SetIsFlying(bool val)
{
	isFlying = val;
}

bool CMario::GetIsFlying() {
	return isFlying;
}

void CMario::SetIsFalling(bool val)
{
	isFalling = val;
}

bool CMario::GetIsFalling() {
	return isFalling;
}

void CMario::SetIsGliding(bool val)
{
	isGliding = val;
}

bool CMario::GetIsGliding() {
	return isGliding;
}

void CMario::SetIsFallingTail(bool val)
{
	isFallingTail = val;
}

bool CMario::GetIsFallingTail() {
	return isFallingTail;
}

void CMario::SetTransforming(bool val)
{
	transforming = val;
}

bool CMario::GetTransforming() {
	return transforming;
}

void CMario::SetStartTransforming(DWORD startTransforming)
{
	this->startTransforming = startTransforming;
}

DWORD CMario::GetStartTransforming() {
	return startTransforming;
}

float CMario::GetOldX()
{
	return oldX;
}

float CMario::GetOldY() {
	return oldY;
}

float CMario::GetVx()
{
	return vx;
}

float CMario::GetVy() {
	return vy;
}

void CMario::turnIntoSmall()
{
	transforming = 1;
	level = MARIO_LEVEL_SMALL;
	y = (y + (MARIO_BIG_BBOX_HEIGHT - MARIO_SMALL_BBOX_HEIGHT));
	SetState(MARIO_STATE_BIG_TO_SMALL);
}

void CMario::turnIntoBig() {
	/*if (level == MARIO_LEVEL_SMALL) {

	}*//*
	if (level == MARIO_LEVEL_BIG) {
		return;
	}*/
	transforming = 1;
	level = MARIO_LEVEL_BIG;
	y = (y - (MARIO_BIG_BBOX_HEIGHT - MARIO_SMALL_BBOX_HEIGHT));
	SetState(MARIO_STATE_BIG_TO_SMALL);
}

/*
	Reset Mario status to the beginning state of a scene
*/
void CMario::Reset()
{
	SetState(MARIO_STATE_IDLE);
	SetLevel(MARIO_LEVEL_SMALL);
	SetPosition(start_x, start_y);
	SetSpeed(0, 0);
}

void CMario::RenderSizeTransforming()
{
	int ani;

	//ani = (nx > 0) ? MARIO_ANI_BIG_TO_SMALL_RIGHT : MARIO_ANI_BIG_TO_SMALL_LEFT;

	if (level == MARIO_LEVEL_BIG) {
		turnIntoSmall();
		//level = MARIO_LEVEL_SMALL;
		ani = (nx > 0) ? MARIO_ANI_SMALL_IDLE_RIGHT : MARIO_ANI_SMALL_IDLE_LEFT;
	}
	else {
		turnIntoBig();
		//level = MARIO_LEVEL_BIG;
		ani = (nx > 0) ? MARIO_ANI_BIG_IDLE_RIGHT : MARIO_ANI_BIG_IDLE_LEFT;
	}

	animation_set->at(ani)->Render(x, y);
}

