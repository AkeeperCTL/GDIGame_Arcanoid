#pragma once

typedef UINT EntityId;

enum ECollideSide
{
	eCS_LEFT,
	eCS_TOP,
	eCS_RIGHT,
	eCS_BOTTOM,

	eCS_LEFT_TOP,
	eCS_RIGHT_TOP,
	eCS_LEFT_BOTTOM,
	eCS_RIGHT_BOTTOM,

	eCS_NOMATTER,
};

enum EGameRectType
{
	eGRT_Default,
	eGRT_Brick,
	eGRT_Player,
	eGRT_Ball,
	eGRT_Text,
};

struct Vec2
{
	Vec2()
	{
		x = 0;
		y = 0;
	}

	Vec2(float _x, float _y) :
		x(_x),
		y(_y)
	{

	}

	float x;
	float y;

	void operator = (const Vec2& vec)
	{
		this->x = vec.x;
		this->y = vec.y;
	}

	bool operator == (const Vec2& vec) const
	{
		return this->x == vec.x && this->y == vec.y;
	}

	inline bool isZero()
	{
		return this->x == 0 && this->y == 0;
	}

	//Vec2& operator * (const float num)
	//{
	//	this->x *= num;
	//	this->y *= num;

	//	return *this;
	//}
};

struct SGameRect
{
	SGameRect()
	{
		hexColor = 0;
		entityId = 0;
		coords = RECT();
		type = eGRT_Default;

		movingSpeed = 0.0f;
		movingDirection = Vec2();
		hide = false;
	}

	SGameRect(const RECT& _coords, uint32_t _color, EntityId _id, EGameRectType _type) :
		coords(_coords),
		hexColor(_color),
		entityId(_id),
		movingSpeed(0.0f),
		movingDirection(Vec2()),
		type(_type),
		hide(false)
	{
	}

	~SGameRect()
	{
		delete this;
	}

	RECT coords;
	uint32_t hexColor;
	EntityId entityId;
	EGameRectType type;

	//No render and no collide
	bool hide;
	
	Vec2 movingDirection; //[-1..0..1]
	float movingSpeed;

	bool operator == (const SGameRect& one) const
	{
		return this->entityId == one.entityId;
	}

	bool operator == (const SGameRect* one) const
	{
		return this->entityId == one->entityId;
	}

	bool operator == (EntityId id) const
	{
		return this->entityId == id;
	}
};

//struct SBlock
//{
//	SBlock()
//	{
//		topCoord = downCoord = rightCoord = leftCoord = 0;
//		aliveState = BlockAliveState_Enabled;
//	}
//
//	enum eBlockAliveState
//	{
//		BlockAliveState_Enabled,
//		BlockAliveState_Disabled
//	};
//
//	int topCoord;
//	int downCoord;
//	int leftCoord;
//	int rightCoord;
//
//	eBlockAliveState aliveState;
//
//	void CreateRectangle(HDC hdc,int left, int top, int right, int buttom);
//
//	//void SetMove(float x);
//};