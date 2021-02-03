#pragma once
#include <assert.h>
#include <stddef.h>

class KNode {
public:
    KNode* m_pNext;
    KNode* m_pPrev;

public:
    KNode();
    virtual ~KNode() {};
    KNode*	GetNext();
    KNode*	GetPrev();
    void	InsertBefore(KNode* pNode);
    void	InsertAfter(KNode* pNode);
    void	Remove();
    int		IsLinked();
};

class HostNode : public KNode {
public:
    void* m_host;

public:
    HostNode();
    virtual ~HostNode() {}
    void* GetHost();
    void SetHost(void* host);
};

inline HostNode::HostNode() :
    m_host(nullptr) {
}

inline void* HostNode::GetHost() {
    return m_host;
}

inline void HostNode::SetHost(void* host) {
    m_host = host;
}

inline KNode::KNode() {
    m_pNext = NULL;
    m_pPrev = NULL;
}

inline KNode* KNode::GetNext() {
    if (m_pNext && m_pNext->m_pNext)
        return m_pNext;
    return NULL;
}

inline KNode* KNode::GetPrev() {
    if (m_pPrev && m_pPrev->m_pPrev)
        return m_pPrev;
    return NULL;
}

inline void KNode::InsertBefore(KNode *pNode) {
    assert(m_pPrev);
    if (!pNode || !m_pPrev)
        return;

    if (pNode->m_pNext || pNode->m_pPrev) {
        assert(false);
    }

    pNode->m_pPrev = m_pPrev;
    pNode->m_pNext = this;
    m_pPrev->m_pNext = pNode;
    m_pPrev = pNode;
}

inline void KNode::InsertAfter(KNode *pNode) {
    assert(m_pNext);
    if (!pNode || !m_pNext)
        return;
    pNode->m_pPrev = this;
    pNode->m_pNext = m_pNext;
    m_pNext->m_pPrev = pNode;
    m_pNext = pNode;
}

inline void KNode::Remove() {
    assert(m_pPrev);
    assert(m_pNext);
    if (m_pPrev)
        m_pPrev->m_pNext = m_pNext;
    if (m_pNext)
        m_pNext->m_pPrev = m_pPrev;
    m_pPrev = NULL;
    m_pNext = NULL;
}

inline int KNode::IsLinked() {
    return (m_pPrev && m_pNext);
}