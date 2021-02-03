#pragma once
#include "knode.h"


class KList {
public:
    KNode	m_ListHead;
    KNode	m_ListTail;
public:
    KList();
    void    Clear();
    KNode*	GetHead();
    KNode*	GetTail();
    void    AddHead(KNode *pNode);
    void    AddTail(KNode *pNode);
    KNode*	RemoveHead();
    KNode*	RemoveTail();
    int		IsEmpty();
    long	GetNodeCount();
    void    AppendTail(KList* list);
};

inline KList::KList() {
    m_ListHead.m_pNext = &m_ListTail;
    m_ListTail.m_pPrev = &m_ListHead;
}

inline void KList::Clear() {
    m_ListHead.m_pNext = &m_ListTail;
    m_ListTail.m_pPrev = &m_ListHead;
}

inline int KList::IsEmpty() {
    return (m_ListHead.GetNext() == NULL);
}

inline KNode* KList::GetHead() {
    return m_ListHead.GetNext();
}

inline KNode* KList::GetTail() {
    return m_ListTail.GetPrev();
}

inline void KList::AddHead(KNode *pNode) {
    m_ListHead.InsertAfter(pNode);
}

inline void KList::AddTail(KNode *pNode) {
    m_ListTail.InsertBefore(pNode);
}

inline KNode* KList::RemoveHead() {
    KNode* pNode = m_ListHead.GetNext();
    if (pNode)
        pNode->Remove();
    return pNode;
}

inline KNode* KList::RemoveTail() {
    KNode* pNode = m_ListTail.GetPrev();
    if (pNode)
        pNode->Remove();
    return pNode;
}

inline long KList::GetNodeCount() {
    long nNode = 0;
    KNode* pNode = GetHead();
    while (pNode) {
        pNode = pNode->GetNext();
        nNode++;
    }
    return nNode;
}

inline void KList::AppendTail(KList* list) {
    assert(this != list);
    KNode* rbegin = list->GetHead();
    KNode* rend = list->GetTail();
    if (rbegin != nullptr && rend != nullptr) {
        rbegin->m_pPrev = m_ListTail.m_pPrev;
        m_ListTail.m_pPrev->m_pNext = rbegin;
        rend->m_pNext = &m_ListTail;
        m_ListTail.m_pPrev = rend;
        list->m_ListHead.m_pNext = &list->m_ListTail;
        list->m_ListTail.m_pPrev = &list->m_ListHead;
    }
}
