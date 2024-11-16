#include <gtest/gtest.h>
#include "Component.hpp"
#include "System.hpp"
#include "ECS.cpp"
#include <bitset>
#include <typeindex>

class ComponentPoolTest : public testing::Test
{
protected:
    struct Position
    {
        double x;
        double y;
    
        void Set(double x, double y)
        {
            this->x = x;
            this->y = y;
        }
    };  
};

TEST_F(ComponentPoolTest, AddingComponent) {
    ComponentPool<Position> comP;
    comP.AddComponent(0);
    EXPECT_ANY_THROW(comP.AddComponent(0));
}

TEST_F(ComponentPoolTest, DeletingComponent)
{
    ComponentPool<Position> comP;
    EXPECT_FALSE(comP.TryDeleteComponent(0)) << "You can delete non-existent component";
    EXPECT_ANY_THROW(comP.DeleteComponent(0)) << "You deleted non-existent component";
    comP.AddComponent(0);
    comP.DeleteComponent(0);
    EXPECT_FALSE(comP.TryGetComponent(0).has_value());

}

TEST_F(ComponentPoolTest, GettingComponent)
{
    ComponentPool<Position> comP;
    EXPECT_FALSE(comP.TryGetComponent(0).has_value()) << "You have access to non-existent component";
    auto& pos = comP.AddComponent(0);
    EXPECT_TRUE(comP.TryGetComponent(0).has_value()) << "You don't have access to existing component";
    pos.Set(6.0, 9.0);
    auto& pos2 = comP.GetComponent(0);
    EXPECT_DOUBLE_EQ(pos.x, pos2.x);
    EXPECT_DOUBLE_EQ(pos.y, pos2.y);
}

TEST_F(ComponentPoolTest, ManipulatingManyComponents)
{
    ComponentPool<Position> comP;
    for(int i=0; i<15; i++)
    {
        try {
            auto& pos = comP.AddComponent(i);
            pos.Set(i, i);
        }
        catch (int ex) {
            ASSERT_TRUE(false) << "Adding component resulted in failure";
        }
    }
    
    ASSERT_TRUE(comP.TryDeleteComponent(6)) << "You can't delete 6th component";
    ASSERT_TRUE(comP.TryDeleteComponent(9)) << "You can't delete 9th component";

    EXPECT_FALSE(comP.TryGetComponent(6).has_value()) << "You have access to non-existent component";
    
    try {
        auto& pos = comP.AddComponent(6);
        pos.Set(6.0, 9.0);
    }
    catch (int ex) {
        ASSERT_TRUE(false) << "Adding 6th component resulted in failure";
    }

    EXPECT_TRUE(comP.TryGetComponent(6).has_value()) << "You don't have access to 6th component";
    
    for(int i=0; i<15; i++)
    {
        if(comP.TryGetComponent(i).has_value())
        {
            auto& pos = comP.GetComponent(i);
            if(i != 6)
            {
                EXPECT_DOUBLE_EQ(pos.x, static_cast<double>(i));
                EXPECT_DOUBLE_EQ(pos.y, static_cast<double>(i));
            }
            else
            {
                EXPECT_DOUBLE_EQ(pos.x, 6.0);
                EXPECT_DOUBLE_EQ(pos.y, 9.0);
            }
        }
    }
}

class SystemTest : public testing::Test
{
protected:
    void SetUp() override
    {
        typeToId[std::type_index(typeid(Position))] = 0;
        typeToId[std::type_index(typeid(Rotation))] = 1;
        
        signatures[0].set(typeToId[std::type_index(typeid(Position))]);
        signatures[0].set(typeToId[std::type_index(typeid(Rotation))]);
        
        signatures[1].set(typeToId[std::type_index(typeid(Rotation))]);
    }

    struct Position
    {
        double x;
        double y;
      
        void Set(double x, double y)
        {
            this->x = x;
            this->y = y;
        }
    };

    struct Rotation
    {
        double deg;
    };

    class EmptySys : public System
    {
    public:
        void SetSignature(std::unordered_map<std::type_index, ComponentPoolId>& compToId) override{}
        void Update() override {}
    };

    class DummySys1 : public System
    {
    public:
        int EntitySize()
        {
            return entities.size();
        }
        
        void SetSignature(std::unordered_map<std::type_index, ComponentPoolId>& compToId) override
        {
            systemSignature.set(compToId[std::type_index(typeid(Position))]);
        }

        void Update() override {}
    };
    
    class DummySys2 : public System
    {
    public:
        int EntitySize()
        {
            return entities.size();
        }
        
        void SetSignature(std::unordered_map<std::type_index, ComponentPoolId>& compToId) override
        {
            systemSignature.set(compToId[std::type_index(typeid(Position))]);
            systemSignature.set(compToId[std::type_index(typeid(Rotation))]);
        }

        void Update() override {}
    };

    std::array<Signature, MAX_ENTITY_COUNT> signatures;
    std::unordered_map<std::type_index, uint32_t> typeToId;
};

TEST_F(SystemTest, SystemInitialization)
{
    EmptySys sys1;
    EXPECT_ANY_THROW(sys1.Init(signatures, typeToId));

    DummySys2 sys2;
    sys2.Init(signatures, typeToId);

    EXPECT_EQ(sys2.EntitySize(), 1) << "Wrong entity count";
    EXPECT_TRUE(sys2.CheckIfEntitySubscribed(0)) << "Entity with right signature unsubscribed";
    EXPECT_FALSE(sys2.CheckIfEntitySubscribed(1)) << "Entity with wrong signature subscribed";

}

TEST_F(SystemTest, ChangingEntitySignature)
{
    DummySys2 sys;
    sys.Init(signatures, typeToId);

    signatures[1].set(typeToId[std::type_index(typeid(Position))]);
    sys.OnEntitySignatureChanged(1, signatures[1]);

    EXPECT_EQ(sys.EntitySize(), 2) << "Wrong entity count";
    EXPECT_TRUE(sys.CheckIfEntitySubscribed(1)) << "Entity with right signature unsubscribed";

    signatures[0].reset(typeToId[std::type_index(typeid(Position))]);
    sys.OnEntitySignatureChanged(0, signatures[0]);

    EXPECT_EQ(sys.EntitySize(), 1) << "Wrong entity count";
    EXPECT_FALSE(sys.CheckIfEntitySubscribed(0)) << "Entity with wrong signature subscribed";

}

TEST_F(SystemTest, DeletingEntity)
{
    DummySys2 sys;
    sys.Init(signatures, typeToId);
    EXPECT_ANY_THROW(sys.OnEntityDestroyed(2)) << "Deleted non-existent entity";

    sys.OnEntityDestroyed(0);
    EXPECT_EQ(sys.EntitySize(), 0) << "Wrong entity count";
    EXPECT_FALSE(sys.CheckIfEntitySubscribed(0)) << "Deleted entity is subscribed";
}