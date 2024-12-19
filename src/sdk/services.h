class CPlayerPawnComponent
{
public:
    virtual ~CPlayerPawnComponent() = 0;

private:
    [[maybe_unused]] unsigned char __pad0008[0x28]; // 0x8
public:
    void* m_pPawn; // 0x30
};

class CPlayer_MovementServices : public CPlayerPawnComponent
{
};