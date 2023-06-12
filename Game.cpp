
#include <cstdlib>
#include <map>
#include <iostream>
#include <Windows.h>

#include "Engine.h"
#include "Game.h"

#include <unordered_map>

#include "vector"

//
//  You are free to modify this file
//

//  is_key_pressed(int button_vk_code) - check if a key is pressed,
//                                       use keycodes (VK_SPACE, VK_RIGHT, VK_LEFT, VK_UP, VK_DOWN, 'A', 'B')
//
//  get_cursor_x(), get_cursor_y() - get mouse cursor position
//  is_mouse_button_pressed(int button) - check if mouse button is pressed (0 - left button, 1 - right button)
//  clear_buffer() - set all pixels in buffer to 'black'
//  is_window_active() - returns true if window is active
//  schedule_quit_game() - quit game after act()

//Window
RECT g_clientRect;
HWND g_hwnd;
HDC g_hdc;

HGDIOBJ g_oldSelectedObject;
HBRUSH g_debugBrush;

//Player
//RECT g_playerRect;
//POINT g_playerRectPoint;
int g_playerLives;
float g_playerSpeed;
float g_ballSpeed;
EntityId g_playerRectId;
EntityId g_ballRectId;

//Enemies
RECT g_brickRect;

//Game
std::unordered_map<EntityId, SGameRect*> g_gameRects;
float g_updateDelay;

// float g_lastUpdateTime;

// #define BUFFER_FILL_RECT(inputBuffer, inputRect, hexColor)\
// for (int y = (inputRect).top; y != (inputRect).bottom; y++)\
// {\
// 	for (int x = (inputRect).left; x < (inputRect).right; x++)\
// 	{\
// 		(inputBuffer)[y][x] = hexColor;\
// 	}\
// }\

#define PROCESS_BALL_BORDER_COLLISION(side, movDir)\
if ((side) == eCS_LEFT || (side) == eCS_RIGHT)\
	(movDir).x *= -1;\
else if ((side) == eCS_TOP || (side) == eCS_BOTTOM)\
	(movDir).y *= -1;\

#define PROCESS_PLAYER_BORDER_COLLISION(side, movDir)\
if ((side) == eCS_LEFT || (side) == eCS_RIGHT)\
	(movDir).x = 0;\
if ((side) == eCS_TOP || (side) == eCS_BOTTOM)\
	(movDir).y = 0;\



EntityId CreateGameRect(const RECT& coords, uint32_t hexColor, EGameRectType type, Vec2 movDir = Vec2(0, 0), float movSpeed = 0);
SGameRect* GetGameRect(EntityId id);

void Update(float frametime);
void OnCollide(EntityId id1, EntityId id2, ECollideSide side);
bool IsCollide(const SGameRect* pGameRect1, const SGameRect* pGameRect2);
bool ApplyMove(EntityId GameRectid, float speed, const Vec2 & dir);

bool RemoveGameRect(const EntityId id)
{
	//new O(log n)
	const auto it = g_gameRects.find(id);
	if(it != g_gameRects.end())
		g_gameRects.erase(it);
	
	//Old O(n^2) complexity
	// for (auto pGameRect : g_gameRects)
	// {
	// 	if (pGameRect->entityId == id)
	// 	{
	// 		auto iter = std::find(g_gameRects.begin(), g_gameRects.end(), pGameRect);
	// 		if (iter != g_gameRects.end())
	// 		{
	// 			g_gameRects.erase(iter);
	// 			delete pGameRect;
	//
	// 			return true;
	// 		}
	//
	// 	}
	// }
	//
	
	return false;
}

void RemoveGameRects()
{
	g_playerRectId = 0;
	g_ballRectId = 0;
	g_gameRects.clear();
}

void RecreateGameRects()
{
	RemoveGameRects();
	//g_gameRects.clear();

	//create player
	auto playerRect = RECT();
	if (g_playerRectId == 0)
	{
		playerRect.left = SCREEN_WIDTH / 2.5f;
		playerRect.top = 600;
		playerRect.right = playerRect.left + 200;
		playerRect.bottom = playerRect.top + 15;

		g_playerRectId = CreateGameRect(playerRect, 0X78DBE2, eGRT_Player);
	}

	//create ball
	if (g_ballRectId == 0)
	{
		auto ballRect = RECT();
		ballRect.left = playerRect.left + ((playerRect.right - playerRect.left) / 2);
		ballRect.top = playerRect.top - (playerRect.bottom - playerRect.top) - 10;
		ballRect.right = ballRect.left + 20;
		ballRect.bottom = ballRect.top + 20;

		g_ballRectId = CreateGameRect(ballRect, 0X78DBE2, eGRT_Ball, Vec2(1, -1), g_ballSpeed);
	}

	//create bricks
	constexpr int brickWidth = 62;
	constexpr int brickHeight = 30;
	constexpr int brickStep = 10;
	constexpr int bricksCountX = 14;
	constexpr int bricksCountY = 5;

	for (int y = 0; y < bricksCountY; y++)
	{
		for (int x = 0; x < bricksCountX; x++)
		{
			g_brickRect.left = g_clientRect.left + 14 + x * brickWidth + x * brickStep;
			g_brickRect.top = g_clientRect.top + 14 + y * brickHeight + y * brickStep;
			g_brickRect.right = g_brickRect.left + brickWidth;
			g_brickRect.bottom = g_brickRect.top + brickHeight;

			CreateGameRect(g_brickRect, 0XFF4A24, eGRT_Brick);
		}
	}

	//create text
	//RECT livesRect = RECT();
	//livesRect.left = SCREEN_WIDTH - (SCREEN_WIDTH / 4.0f);
	//livesRect.top = 700;
	//livesRect.right = livesRect.left + 90;
	//livesRect.bottom = livesRect.top + 30;

	//CreateGameRect(livesRect, 0X00FF00, eGRT_Text);
}

void Restart(const int sleeptime)
{
	Sleep(sleeptime);
	RecreateGameRects();
}

void Update(float frametime)
{
	const auto pPlayerGameRect = GetGameRect(g_playerRectId);
	if (!pPlayerGameRect)
		return;

	//const auto* playerCoords = &pPlayerGameRect->coords;
	//RECT intersection;

	for (const auto pair : g_gameRects)
	{
		const auto pGameRect = pair.second;
		if (!pGameRect)
			continue;

		if (pGameRect->hide)
			continue;
		
		auto* coords = &pGameRect->coords;
		auto* movDir = &pGameRect->movingDirection;
		const auto movSpeed = pGameRect->movingSpeed;

		const bool moveLeft = movDir->x < 0;
		const bool moveRight = movDir->x > 0;
		const bool moveTop = movDir->y < 0;
		const bool moveBottom = movDir->y > 0;
		const bool isMoving = !movDir->isZero();
	
		if (moveLeft)
		{
			if (coords->left <= g_clientRect.left + 20)
				OnCollide(pGameRect->entityId, -1, eCS_LEFT);
		}
		else if (moveRight)
		{
			if (coords->right >= g_clientRect.right - 20)
				OnCollide(pGameRect->entityId, -1, eCS_RIGHT);
		}

		if (moveTop)
		{
			if (coords->top <= g_clientRect.top + 20)
				OnCollide(pGameRect->entityId, -1, eCS_TOP);
		}
		else if (moveBottom)
		{
			if (coords->bottom >= g_clientRect.bottom - 20)
				OnCollide(pGameRect->entityId, -1, eCS_BOTTOM);
		}

		for (const auto colliderPair : g_gameRects)
		{
			const auto pColliderRect = colliderPair.second;
			if (!pColliderRect)
				continue;
			
			if (pGameRect->entityId == pColliderRect->entityId)
				continue;

			if (pColliderRect->hide)
				continue;
		
			 if (IsCollide(pGameRect, pColliderRect))
			 {
				ECollideSide collideSide = eCS_NOMATTER;
			
				if (moveTop && collideSide == eCS_NOMATTER)
					collideSide = eCS_BOTTOM;
				if (moveBottom && collideSide == eCS_NOMATTER)
					collideSide = eCS_TOP;
			
				if (moveLeft && collideSide == eCS_NOMATTER)
					collideSide = eCS_RIGHT;
				if (moveRight && collideSide == eCS_NOMATTER)
					collideSide = eCS_LEFT;
			
			 	OnCollide(pGameRect->entityId, pColliderRect->entityId, collideSide);
			 }
		}

		if (isMoving)
			OffsetRect(coords, movDir->x * movSpeed, movDir->y * movSpeed);
	}
}

//The 1 crashed into the 2 from side1
void OnCollide(const EntityId id1, const EntityId id2, const ECollideSide side1)
{
	const auto pGameRect1 = GetGameRect(id1);
	if (!pGameRect1)
		return;

	bool collideWithBorder = false;

	const auto pGameRect2 = GetGameRect(id2);
	if (!pGameRect2 || id2 == -1)
		collideWithBorder = true;

	if (!collideWithBorder)
	{		
		//Rect to rect collision

		if (!pGameRect2)
			return;

		if (pGameRect1->type == eGRT_Ball)
		{
			PROCESS_BALL_BORDER_COLLISION(side1, pGameRect1->movingDirection);

			if (pGameRect2->type == eGRT_Brick)
				pGameRect2->hide = true;
				//RemoveGameRect(pGameRect2->entityId);
		}
	}
	else
	{
		//Game Border collision

		if (pGameRect1->type == eGRT_Ball)
		{
			PROCESS_BALL_BORDER_COLLISION(side1, pGameRect1->movingDirection);

			if (side1 == eCS_BOTTOM)
			{
				Restart(1000);
			}
		}
		else if (pGameRect1->type == eGRT_Player)
		{
			PROCESS_PLAYER_BORDER_COLLISION(side1, pGameRect1->movingDirection);
		}
		else
		{

		}
	}
}

bool IsCollide(const SGameRect* pGameRect1, const SGameRect* pGameRect2)
{
	if (!pGameRect1 || !pGameRect2)
		return false;

	RECT intersect;
	return (IntersectRect(&intersect, &pGameRect1->coords, &pGameRect2->coords));
}

void ProcessKeysInput(int playerLives)
{
	if (g_playerLives <= 0)
		return;

	Vec2 movDir;
	float movSpeed = 0;
	bool moveKeyPressed = false;

	if (is_key_pressed(VK_UP))
	{
		movDir.y = -1;
		moveKeyPressed = true;
	}

	if (is_key_pressed(VK_DOWN))
	{
		movDir.y = 1;
		moveKeyPressed = true;
	}

	if (is_key_pressed(VK_RIGHT))
	{
		movDir.x = 1;
		moveKeyPressed = true;
	}

	if (is_key_pressed(VK_LEFT))
	{
		movDir.x = -1;
		moveKeyPressed = true;
	}

	if (!moveKeyPressed)
	{
		movDir.x = 0;
		movSpeed = 0.0f;
	}

	movDir.y = 0;

	if (moveKeyPressed)
		movSpeed = g_playerSpeed;

	ApplyMove(g_playerRectId, movSpeed, movDir);
}

bool ApplyMove(const EntityId GameRectid, const float speed, const Vec2& dir)
{
	const auto it = g_gameRects.find(GameRectid);
	if (it != g_gameRects.end())
	{
		const auto pGameRect = it->second;
		if (pGameRect)
		{
			pGameRect->movingDirection = dir;
			pGameRect->movingSpeed = speed;
		}
	}

	return false;
}

SGameRect* GetGameRect(const EntityId id)
{
	const auto it = g_gameRects.find(id);
	if (it != g_gameRects.end())
		return it->second;

	return nullptr;
}

EntityId CreateGameRect(const RECT& coords, const uint32_t hexColor, const EGameRectType type, const Vec2 movDir, const float movSpeed)
{
	auto* pGameRect = new SGameRect();
	pGameRect->coords = coords;
	pGameRect->hexColor = hexColor;
	pGameRect->entityId = g_gameRects.size() + 1;
	pGameRect->movingSpeed = movSpeed;
	pGameRect->movingDirection = movDir;
	pGameRect->type = type;

	g_gameRects[pGameRect->entityId] = pGameRect;
	return pGameRect->entityId;
}

// initialize game data in this function
void initialize()
{	
	g_playerLives = 3;
	g_playerSpeed = 2.0f;
	g_ballSpeed = 2.0f;
	g_updateDelay = 0.5f;

	g_hwnd = FindWindow(0, "Game");
	if (g_hwnd)
	{
		GetClientRect(g_hwnd, &g_clientRect);

		g_hdc = GetWindowDC(g_hwnd);
		if (g_hdc)
		{
			g_debugBrush = CreateSolidBrush(RGB(255,255,255));
			RecreateGameRects();
		}
	}
}

// this function is called to update game data,
// dt - time elapsed since the previous update (in seconds)
void act(const float dt)
{
	const float frametime = dt;

	//Stop gamemode when we lose
	if (g_playerLives <= 0)
		return;
	
	if (!is_window_active())
		return;

	ProcessKeysInput(g_playerLives);
	Update(frametime);

	// clear the backbuffer so that the animation does not leave a trace
	clear_buffer();
}

// fill buffer in this function
// uint32_t buffer[SCREEN_HEIGHT][SCREEN_WIDTH] - is an array of 32-bit colors (8 bits per R, G, B)
void draw()
{
	for (const auto pair : g_gameRects)
	{
		const auto pGameRect = pair.second;
		if (!pGameRect)
			continue;

		if (pGameRect->hide)
			continue;
		
		const RECT coords = pGameRect->coords;
		const uint32_t hexColor = pGameRect->hexColor;

		const HDC hdcc = CreateCompatibleDC(g_hdc);
		const HBITMAP bm = CreateCompatibleBitmap(g_hdc, coords.right, coords.bottom);
		SelectObject(hdcc, bm);

		BITMAPINFO bif;
		ZeroMemory(&bif, sizeof(BITMAPINFO));

		bif.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bif.bmiHeader.biBitCount = 32;
		bif.bmiHeader.biWidth = coords.right;
		bif.bmiHeader.biHeight = coords.bottom;

		//BUFFER_FILL_RECT(buffer, coords, hexColor);
		for (int y = coords.top; y <= coords.bottom; y++)
		{
			for (int x = coords.left; x <= coords.right; x++)
			{
				buffer[y][x] = hexColor;
			}
		}

		GetDIBits(hdcc, bm, 0, coords.bottom, buffer, &bif, DIB_RGB_COLORS);
		SetDIBitsToDevice(g_hdc, 0, 0, coords.right, coords.bottom, 0, 0, 0, coords.bottom, buffer, &bif, DIB_RGB_COLORS);

		DeleteObject(bm);
		DeleteDC(hdcc);
	}
}

// free game data in this function
void finalize()
{
	DeleteObject(g_debugBrush);

	// clear backbuffer
	memset(buffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint32_t));
}