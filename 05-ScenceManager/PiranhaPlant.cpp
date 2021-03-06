#include "PiranhaPlant.h"
#include "Utils.h"

#include <math.h>

PiranhaPlant::PiranhaPlant(float pipeX, float pipeY, float pipeWidth, float pipeHeight, LPGAMEOBJECT player, int level)
{
	this->player = player;

	this->pipeX = pipeX;
	this->pipeY = pipeY;
	this->pipeWidth = pipeWidth;
	this->pipeHeight = pipeHeight;

	SetState(PIRANHAPLAN_STATE_AWAKE);
	climax = (int)(pipeY - ((level == PIRANHAPLANT_LEVEL_BIG) ? PIRANHAPLANT_BBOX_HEIGHT : PIRANHAPLANT_BBOX_HEIGHT_SMALL));

	y = pipeY;

	this->level = level;

	vx = 0;
	vy = -0.065f;
	isSleeping = 1;
	startResting = (DWORD)GetTickCount64();
	startAttacking = 0;
}

void PiranhaPlant::Render()
{
	float camX, camY;
	int screenWidth, screenHeight;
	CGame* gameInstance = CGame::GetInstance();

	camX = gameInstance->GetCamX();
	camY = gameInstance->GetCamY();
	screenWidth = gameInstance->GetScreenWidth();
	screenHeight = gameInstance->GetScreenHeight();

	if (x + PIRANHAPLANT_BBOX_WIDTH < camX || x>camX + screenWidth ||
		y + PIRANHAPLANT_BBOX_HEIGHT<camY || y>camY + screenHeight ||
		y >= pipeY) {
		return;
	}

	int ani = PIRANHAPLANT_ANI_BOTLEFT;

	float playerLeft, playerTop, playerRight, playerBottom;

	player->GetBoundingBox(playerLeft, playerTop, playerRight, playerBottom);

	if (state == PIRANHAPLANT_STATE_SLEEP) {
		ani = PIRANHAPLANT_ANI_SLEEP;
	}
	else if (playerLeft < this->x) {
		dirX = -1;
		if (playerBottom <= this->y) {
			dirY = -1;
			ani = (this->y == climax) ? PIRANHAPLANT_ANI_TOPLEFT_STILL : PIRANHAPLANT_ANI_TOPLEFT;
		}
		else {
			dirY = 1;
			ani = (this->y == climax) ? PIRANHAPLANT_ANI_BOTLEFT_STILL : ani;
		}
	}
	else {
		dirX = 1;
		if (playerBottom <= this->y) {
			dirY = -1;
			ani = (this->y == climax) ? PIRANHAPLANT_ANI_TOPRIGHT_STILL : PIRANHAPLANT_ANI_TOPRIGHT;
		}
		else {
			dirY = 1;
			ani = (this->y == climax) ? PIRANHAPLANT_ANI_BOTRIGHT_STILL : PIRANHAPLANT_ANI_BOTRIGHT;
		}
	}
	ani += level * PIRANHAPLANT_ANI_NUM;

	if (state == PIRANHAPLANT_STATE_DIE) {
		ani = PIRANHAPLANT_ANI_DIE;
	}

	currAni = ani;

	animation_set->at(ani)->Render(x, y);
	//RenderBoundingBox();
}

void PiranhaPlant::Update(DWORD dt, vector<LPGAMEOBJECT>* coObjects)
{
	float camX, camY;
	int screenWidth, screenHeight;
	CGame* gameInstance = CGame::GetInstance();

	camX = gameInstance->GetCamX();
	camY = gameInstance->GetCamY();
	screenWidth = gameInstance->GetScreenWidth();
	screenHeight = gameInstance->GetScreenHeight();

	/*if (y <= climax) {
		vy = 0.25f;
	}
	else if (y >= pipeY) {
		vy = -0.25f;
	}*/

	/*if (y >= pipeY) {
		vy = -0.25f;
	}*/

	if (x + PIRANHAPLANT_BBOX_WIDTH < camX || x > camX + screenWidth ||
		y + PIRANHAPLANT_BBOX_HEIGHT < camY || y > camY + screenHeight) {
		return;
	}

	if (state == PIRANHAPLANT_STATE_DIE && (int)((DWORD)GetTickCount64() - start_die > PIRANHAPLANT_DIE_TIME)) {
		isActive = 0;
		invisible = 1;
	}

	float playerLeft, playerTop, playerRight, playerBottom;

	player->GetBoundingBox(playerLeft, playerTop, playerRight, playerBottom);

	if ((abs(playerRight - pipeX) <= 1 || abs(playerLeft - pipeX - pipeWidth) <= 1) && y >= pipeY) {
		return;
	}
	if (level == PIRANHAPLANT_LEVEL_SMALL && y >= pipeY && playerLeft >= pipeX && playerRight <= pipeY + pipeWidth) {
		return;
	}

	CGameObject::Update(dt);

	x += dx;
	y += dy;

	if (this->y <= climax && !startAttacking) {
		y = (float)climax;
		vy = 0;
		startAttacking = (DWORD)GetTickCount64();
		startResting = 0;
		return;
	}
	else if (this->y > pipeY && !startResting) {
		vy = 0;
		startResting = (DWORD)GetTickCount64();
		startAttacking = 0;
		isSleeping = 1;
	}

	if (GetTickCount64() - startAttacking >= LOCKING_TARGET_TIME && !fired && startAttacking) {
		// fire
		fired = 1;
		throwFlame();
	}

	if (startAttacking && (DWORD)GetTickCount64() - startAttacking >= PIRANHAPLANT_AWAKE_TIME) {
		vy = PIRANHAPLANT_SLEEP_SPEED;
		startAttacking = 0;
	}
	else if (startResting && (DWORD)GetTickCount64() - startResting >= PIRANHAPLANT_REST_TIME) {
		vy = PIRANHAPLANT_AWAKE_SPEED;
		startResting = 0;
		fired = 0;
		isSleeping = 0;
	}

	//CGameObject::Update(dt);
	//DebugOut(L"[Piranha Plant] %f %f", x, y);
}

void PiranhaPlant::GetBoundingBox(float& l, float& t, float& r, float& b)
{
	l = x;
	t = y;
	r = x + PIRANHAPLANT_BBOX_WIDTH;
	b = y + ((level == PIRANHAPLANT_LEVEL_BIG) ? PIRANHAPLANT_BBOX_HEIGHT : PIRANHAPLANT_BBOX_HEIGHT_SMALL);
}

void PiranhaPlant::SetState(int state)
{
	CGameObject::SetState(state);

	if (state == PIRANHAPLANT_STATE_DIE) {
		start_die = (DWORD)GetTickCount64();
		interactivable = 0;
	}
}

void PiranhaPlant::throwFlame()
{
	CAnimationSets* animationSets = CAnimationSets::GetInstance();

	LPGAMEOBJECT obj = new FireBall(dirX, dirY);
	obj->SetPosition(this->x + PIRANHAPLANT_BBOX_WIDTH / 3, this->y);
	obj->SetAnimationSet(animationSets->Get(FLAME_ANI_SET_ID));
	Grid::GetInstance()->putObjectIntoGrid(obj);
}


