#pragma once

#include <playerslot.h>
#include "../entrypoint.h"

#define INVALID_PLAYER_SLOT_INDEX -1
#define INVALID_PLAYER_SLOT CPlayerSlot( INVALID_PLAYER_SLOT_INDEX )

class CTeam;

inline uint PopulationCount(uint32 v)
{
    uint32 const w = v - ((v >> 1) & 0x55555555);
    uint32 const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    return (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

class ICRecipientFilter
{
public:
    virtual			~ICRecipientFilter() {}

    virtual bool	IsInitMessage() const = 0;
    virtual NetChannelBufType_t	GetNetworkBufType() const = 0;
    virtual const CPlayerBitVec& GetRecipients() const = 0;
    int	GetRecipientCount() const {
        const CPlayerBitVec& vec = GetRecipients();

        int nDwordCount = vec.GetNumDWords();
        const uint32* pBase = vec.Base();
        uint32 nCount = 0;
        for (int i = 0; i < nDwordCount; ++i)
        {
            nCount += ::PopulationCount(pBase[i]);
        }
        return nCount;
    }
};

class CRecipientFilter : public ICRecipientFilter
{
public:
    CRecipientFilter(NetChannelBufType_t nBufType = BUF_RELIABLE, bool bInitMessage = false) :
        m_nBufType(nBufType), m_bInitMessage(bInitMessage) {
    }

    CRecipientFilter(ICRecipientFilter* source, CPlayerSlot exceptSlot = INVALID_PLAYER_SLOT)
    {
        m_Recipients = source->GetRecipients();
        m_nBufType = source->GetNetworkBufType();
        m_bInitMessage = source->IsInitMessage();

        if (exceptSlot != INVALID_PLAYER_SLOT)
        {
            m_Recipients.Clear(exceptSlot.Get());
        }
    }

    ~CRecipientFilter() override {}

    NetChannelBufType_t GetNetworkBufType(void) const override { return m_nBufType; }
    bool IsInitMessage(void) const override { return m_bInitMessage; }
    const CPlayerBitVec& GetRecipients(void) const override { return m_Recipients; }

    void SetRecipients(uint64 nRecipients)
    {
        m_Recipients.Set(0UL, static_cast<uint32>(nRecipients & 0xFFFFFFFF));
        m_Recipients.Set(1UL, static_cast<uint32>(nRecipients >> 32));
    }

    void AddRecipient(CPlayerSlot slot)
    {
        if (slot.Get() >= 0 && slot.Get() < ABSOLUTE_PLAYER_LIMIT)
        {
            m_Recipients.Set(slot.Get());
        }
    }

    void RemoveRecipient(CPlayerSlot slot)
    {
        if (slot.Get() >= 0 && slot.Get() < ABSOLUTE_PLAYER_LIMIT)
        {
            m_Recipients.Clear(slot.Get());
        }
    }

    void AddPlayersFromBitMask(const CPlayerBitVec& playerbits)
    {
        int index = playerbits.FindNextSetBit(0);

        while (index > -1)
        {
            AddRecipient(index);

            index = playerbits.FindNextSetBit(index + 1);
        }
    }

    void RemovePlayersFromBitMask(const CPlayerBitVec& playerbits)
    {
        int index = playerbits.FindNextSetBit(0);

        while (index > -1)
        {
            RemoveRecipient(index);

            index = playerbits.FindNextSetBit(index + 1);
        }
    }

public:

    void CopyFrom(const CRecipientFilter& src)
    {
        m_Recipients = src.m_Recipients;
        m_bInitMessage = src.m_bInitMessage;
    }

    void Reset(void)
    {
        m_Recipients.ClearAll();
        m_bInitMessage = false;
    }

    void			MakeInitMessage(void) { m_bInitMessage = true; }
    void			MakeReliable(void) {}
    void			AddAllPlayers(void) {}

    void			AddRecipientsByPVS(const Vector& origin);
    void			RemoveRecipientsByPVS(const Vector& origin);
    void			AddRecipientsByPAS(const Vector& origin);
    void			RemoveAllRecipients(void);
    void			RemoveRecipientByPlayerIndex(int playerindex);
    void			AddRecipientsByTeam(const CTeam* team);
    void			RemoveRecipientsByTeam(const CTeam* team);
    void			RemoveRecipientsNotOnTeam(const CTeam* team);

    void			UsePredictionRules(void);
    bool			IsUsingPredictionRules(void) const;

    bool			IgnorePredictionCull(void) const;
    void			SetIgnorePredictionCull(bool ignore);

    void			RemoveSplitScreenPlayers();
    void			ReplaceSplitScreenPlayersWithOwners();

    void			RemoveDuplicateRecipients();

protected:
    CPlayerBitVec m_Recipients;
    NetChannelBufType_t m_nBufType;
    bool m_bInitMessage;
};

class CSingleRecipientFilter : public CRecipientFilter
{
public:
    CSingleRecipientFilter(CPlayerSlot nRecipientSlot, NetChannelBufType_t nBufType = BUF_RELIABLE, bool bInitMessage = false) :
        CRecipientFilter(nBufType, bInitMessage)
    {
        if (nRecipientSlot.Get() >= 0 && nRecipientSlot.Get() < ABSOLUTE_PLAYER_LIMIT)
        {
            m_Recipients.Set(nRecipientSlot.Get());
        }
    }
};
