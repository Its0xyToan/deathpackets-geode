/**
 * Include the Geode headers.
 */
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class SimplePosition {
	public:
		int x;
		int y;
};

class PacketPlayer {
	public:
		SimplePosition position;
		int playerId;
		bool isCube;
		bool isShip;
		bool isBall;
		bool isSpider;
		bool isSwing;
		bool isWave;
		bool isRobot;
		bool isUfo;
};

class UsefulDeathPlayerData {
	public:
		int frame;

		bool isDual;
		PacketPlayer player1;
		PacketPlayer player2;
};

matjson::Value createJson(UsefulDeathPlayerData data) {
	matjson::Value json;

	json.set("frame", data.frame);
	json.set("isDual", data.isDual);

	// Player 1
	matjson::Value p1;
	p1.set("playerId", data.player1.playerId);
	matjson::Value pos1;
	pos1.set("x", data.player1.position.x);
	pos1.set("y", data.player1.position.y);
	p1.set("position", pos1);
	p1.set("isCube", data.player1.isCube);
	p1.set("isBall", data.player1.isBall);
	p1.set("isRobot", data.player1.isRobot);
	p1.set("isShip", data.player1.isShip);
	p1.set("isSpider", data.player1.isSpider);
	p1.set("isSwing", data.player1.isSwing);
	p1.set("isUfo", data.player1.isUfo);
	p1.set("isWave", data.player1.isWave);

	json.set("player1", p1);

	if(data.isDual == true) {
		// Player 2
		matjson::Value p2;
		p2.set("playerId", data.player2.playerId);
		matjson::Value pos2;
		pos2.set("x", data.player2.position.x);
		pos2.set("y", data.player2.position.y);
		p2.set("position", pos2);
		p2.set("isCube", data.player2.isCube);
		p2.set("isBall", data.player2.isBall);
		p2.set("isRobot", data.player2.isRobot);
		p2.set("isShip", data.player2.isShip);
		p2.set("isSpider", data.player2.isSpider);
		p2.set("isSwing", data.player2.isSwing);
		p2.set("isUfo", data.player2.isUfo);
		p2.set("isWave", data.player2.isWave);

		json.set("player2", p2);
	}
	
	return json;
};


class $modify(ModifiedPlayerObject, PlayerObject) {
	static void onModify(auto& self) {
		// Hook before QOLMod and DeathMarkers
		if(!self.setHookPriority("PlayerObject::playerDestroyed", -6971)) {
			log::error("Failed to set priority of PlayerObject::playerDestroyed to -6971 (hmmm)");
		}
	}

	struct Fields {
        EventListener<web::WebTask> m_listener;
    };

	void playerDestroyed(bool secondPlr) {
		PlayerObject::playerDestroyed(secondPlr);

		log::info("Sending telemetry data to post api");

		auto game = GameManager::get();
		auto player = PlayLayer::get();

		auto player1 = player->m_player1;
		auto player2 = player->m_player2;

		UsefulDeathPlayerData data;

		data.frame = game->m_gameLayer->m_gameState.m_currentProgress;
		data.isDual = player->m_gameState.m_isDualMode;

		PacketPlayer sp1 = PacketPlayer();
		// Player Mode
		sp1.playerId = 1;
		SimplePosition deathPosition1;
		deathPosition1.x = player1->getPositionX();
		deathPosition1.y = player1->getPositionY();
		sp1.position = deathPosition1;
		sp1.isBall = player1->m_isBall;
		sp1.isShip = player1->m_isShip;
		sp1.isRobot = player1->m_isRobot;
		sp1.isUfo = player1->m_isBird;
		sp1.isSpider = player1->m_isSpider;
		sp1.isWave = player1->m_isDart;
		sp1.isCube = !sp1.isBall && !sp1.isRobot && !sp1.isShip && !sp1.isSpider && !sp1.isSwing && !sp1.isUfo && !sp1.isWave;

		PacketPlayer sp2 = PacketPlayer();
		if(data.isDual) {
			sp2.playerId = 2;
			SimplePosition deathPosition2;
			deathPosition2.x = player2->getPositionX();
			deathPosition2.y = player2->getPositionY();
			sp2.position = deathPosition2;
			sp2.isBall = player2->m_isBall;
			sp2.isShip = player2->m_isShip;
			sp2.isRobot = player2->m_isRobot;
			sp2.isUfo = player2->m_isBird;
			sp2.isSpider = player2->m_isSpider;
			sp2.isWave = player2->m_isDart;

			sp2.isCube = !sp2.isBall && !sp2.isRobot && !sp2.isShip && !sp2.isSpider && !sp2.isSwing && !sp2.isUfo && !sp2.isWave;
		}

		data.player1 = sp1;
		data.player2 = sp2;
		
		matjson::Value json = createJson(data);

		web::WebRequest req = web::WebRequest();

		req.userAgent("Geode Web DeathPackets");
		req.bodyJSON(json);
		req.header("Content-Type", "application/json");
		std::string where = Mod::get()->getSettingValue<std::string>("post_location");
		auto task = req.post(where);

		m_fields->m_listener.bind([] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                log::info("Finished posting");
            } else if (e->isCancelled()) {
                log::error("The request was cancelled for some reason");
            }
        });
		m_fields->m_listener.setFilter(task);
	}
};