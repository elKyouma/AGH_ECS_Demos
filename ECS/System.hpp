#pragma once
#include "ComponentManager.hpp"
#include "Types.hpp"
#include <array>
#include <unordered_set>

class System
{
public:
    void Init(Signature *signatures,
              ComponentManager* compManager)
    {
        this->compManager = compManager;
        SetSignature(systemSignature);
        ASSERT(systemSignature.to_ulong() != 0u);
        
        for(EntityId id = 0; id < MAX_ENTITY_COUNT; id++)
            if((systemSignature.to_ulong() & signatures[id].to_ulong()) == systemSignature.to_ulong())
                entities.emplace(id);
    }

    //TODO: Think about making update protected, and befriending ECS

    virtual void SetSignature(Signature& systemSignature) = 0;
    virtual void Update(const float deltaTime){}
    virtual void Render(){}

    void OnEntityDestroyed(const EntityId entity)
    {
        ASSERT(entities.find(entity) != entities.end())
        entities.erase(entity);
    }

    void OnEntitySignatureChanged(const EntityId entity, const Signature newSignature)
    {
        if(entities.find(entity) == entities.end())
        {     
            if((systemSignature.to_ulong() & newSignature.to_ulong()) == systemSignature.to_ulong())
                entities.emplace(entity);
        }
        else 
            if((systemSignature.to_ulong() & newSignature.to_ulong()) != systemSignature.to_ulong())
                entities.erase(entity);
    }

    #ifdef IN_TEST
    bool CheckIfEntitySubscribed(const EntityId entity) 
    {
        return entities.find(entity) != entities.end();
    }
    #endif // IN_TEST
    
    virtual ~System(){};

protected:
    std::unordered_set<EntityId> entities;
    ComponentManager* compManager;

private: 
    Signature systemSignature;
};
