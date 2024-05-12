#include <SFML/Graphics.hpp>
#include <iostream>
#include "Board.hpp"
#include "Hero.hpp"
#include "Enemy.hpp"
#include "Button.hpp"


class GameManager
{
public:
	explicit GameManager(void) = default;
	explicit GameManager(sf::RenderWindow& window)
	:window(window),
	board_width(4), 
	board_height(3),
	cur_level(1),
	num_models_of_enemy(1),
	mouse_button_is_pressed(false),
	move_direct_x(0.0f), move_direct_y(0.0f)
	{
		// загружаем статическиe переменные для работы классов
		if (!AKR::loadResources())
			std::cout << "AKR load failed\n";

		if (!Board::loadResources())
			std::cout << "BOARD load failed\n";

		if (!Hero::loadResources())
			std::cout << "HERO load failed\n";

		if (!Enemy::loadResources())
			std::cout << "ENEMY load failed\n";

		if (!Bomb::loadResources())
			std::cout << "Bomb load failed\n";

		if (!font.loadFromFile("Roboto-Black.ttf"))
			std::cout << "FONT load failed\n";

		if (!Button::loadResources())
			std::cout << "BUTTON load failed\n";

		// Подготовка sf::Text
		settings_text(people_info_text);
		settings_text(gun_info_text);
		settings_text(title1_text, sf::Color::Red);
		settings_text(title2_text, sf::Color::Red);
		settings_text(cash_text, sf::Color::Yellow);
		// определение переменных
		// определяем игровое поле (изначально оно на весь экран)todo
		sf::Vector2u window_size = window.getSize();
		sf::FloatRect board_pos(0, 0, window_size.x - 250, window_size.y);
		b = Board(board_pos);
		h = Hero();

		info_pannel_pos = sf::FloatRect(window_size.x - 250, 0, 250, window_size.y);
		// инициализируем кнопки
		button_init();
	}
	void run(void)
	{
		// пока мы не проиграли
		bool game_is_over = false;
		while (!game_is_over)
		{
			init();	// генерируем новый уровень
			while (true)
			{
				// ввод
				input();
				// обработка
				treatment();
				// вывод
				output();
			}
			new_level();
		}
		// вывод результата игры todo
	}
private:
	// обрабатывает ввод пользователя
	void input(void)
	{
		// слушаем события
	    sf::Event event;
	    while (window.pollEvent(event))
	    {
	    	switch (event.type)
	    	{
	        // отслеживание закрытия окна
	    	case sf::Event::Closed:
	    		window.close();
	    		break;
	        // отслеживание нажатия кнопок
	    	case sf::Event::KeyPressed:
	    		switch (event.key.code)
	    		{
	            // нажатие кнопок перемещения персонажа (WASD)
	    		case sf::Keyboard::W:
	                move_direct_y = -1.0f;
	                break;
	            case sf::Keyboard::D:
	            	move_direct_x = 1.0f;
	            	break;
	            case sf::Keyboard::S:
	            	move_direct_y = 1.0f;
	            	break;
	            case sf::Keyboard::A:
	            	move_direct_x = -1.0f;
	            	break;
	        	// отслеживаем нажатие кнопки 'B'
	            case sf::Keyboard::B:
		            h.planting(clock.getElapsedTime().asMilliseconds());
	            	break;
	            // отсележиваем нажатие пробела
	            case sf::Keyboard::Space:
	            	make_board_boom();
	            	break;
	            default: 
	            	break;
	    		}
	    		break;
	        // отслеживание отпускания клавиш
	    	case sf::Event::KeyReleased:
	    		switch (event.key.code)
	    		{
		    		case sf::Keyboard::W:
		                move_direct_y = 0.0f;
		                break;
		            case sf::Keyboard::D:
		            	move_direct_x = 0.0f;
		            	break;
		            case sf::Keyboard::S:
		            	move_direct_y = 0.0f;
		            	break;
		            case sf::Keyboard::A:
		            	move_direct_x = 0.0f;
		            	break;
	        		// отслеживание отпускания кнопки 'B'
		            case sf::Keyboard::B:
		            	h.drop_planting();
		            	break;
		            default:
		            	break;
	    		}
	    		break;

	    	default:
	    		break;
	    	}
	        // отслеживание нажатий мыши
	        if (event.type == sf::Event::MouseButtonPressed && 
	        	event.mouseButton.button == sf::Mouse::Left)
	        {
	        	mouse_button_is_pressed = true;
	        }
	        // отслеживание отпускания мыши
	        else if (event.type == sf::Event::MouseButtonReleased && 
	        	event.mouseButton.button == sf::Mouse::Left)
	        {
	        	mouse_button_is_pressed = false;
	        }
	    }
	}

	// делает обработку ввода
	void treatment(void)
	{
		// эксплуатируем hero
		// разворачиваемся в нужном направлении
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f globalMousePos = window.mapPixelToCoords(mousePos);
        h.aim(globalMousePos.x, globalMousePos.y);

        // передвигаемся в соответсвии с нажатыми клавишами
        __int64_t cur_time = clock.getElapsedTime().asMilliseconds();
        h.moving(
        	cur_time,
        	move_direct_x,
        	move_direct_y
        );
        // если происходит пересечение хитбокса с картой => откатываемся
        if (b.check_collision(h.get_hitbox()))
        {
        	h.moving(
	        	cur_time + (cur_time - last_hero_moving_time),
	        	-move_direct_x,
	        	-move_direct_y
        	);
        }
        last_hero_moving_time = cur_time;

        // стреляем если была нажата кнопка мыши
        if (mouse_button_is_pressed)
        {
        	h.shooting(clock.getElapsedTime().asMilliseconds());
        }

        // обновляем патроны в оружии
        h.upgrade_gun(clock.getElapsedTime().asMilliseconds());

        // эксплуатируем персонажей игры
        // находим позицию hero на board
        sf::Vector2i hero_cor = b.calc_pos_on_board(h.get_hitbox());
        // даем enemy из enemies представление об окружающей ситуации
        for (auto &enemy : enemies)
        {
        	sf::Vector2i enemy_cor = b.calc_pos_on_board(enemy.get_hitbox(), enemy.get_direct());
        	enemy.set_path_to_hero(b.navigator(enemy_cor, hero_cor));
        	enemy.set_cooridinate(enemy_cor);
        	enemy.set_hero_pos(h.get_hitbox());
        }
        // целемся стреляем двигаемся при необходимости
        for (auto &enemy : enemies)
        {
        	enemy.aim();
        	enemy.shooting(clock.getElapsedTime().asMilliseconds());
        	enemy.moving(clock.getElapsedTime().asMilliseconds());
        	enemy.upgrade_gun(clock.getElapsedTime().asMilliseconds());
        }

        // делаем файтинг
        // проверяем столкновение enemy с пулями hero
        auto hero_bullets = h.get_bullets();
        for (auto i = hero_bullets.first; i != hero_bullets.second; ++i)
        {
        	// если есть пересечение c врагом
        	for (auto &enemy : enemies)
        	{
        		if (enemy.check_collision(*i, clock.getElapsedTime().asMilliseconds()))
        		{
        			// уменьшаем текущее здоровье врага на урон hero
        			enemy.change_cur_health(- h.get_damage());
        		}
        	}
        }

        // проверяем столкновение hero c пулями enemy
      	for (auto &enemy : enemies)
      	{
      		auto enemy_bullets = enemy.get_bullets();
      		for (auto i = enemy_bullets.first; i != enemy_bullets.second; ++i)
      		{
      			if (h.check_collision(*i, clock.getElapsedTime().asMilliseconds()))
      			{
      				h.change_cur_health(- enemy.get_damage());
      			}
      		}
      	}
      	// ПРОВЕРКА СТОЛКНОВЕНИЙ ПУЛЬ СО СТЕНАМИ ЛАБИРИНТА
      	// проверяем столкновение пуль hero со стенами лабиринта
      	for (auto i = hero_bullets.first; i != hero_bullets.second; ++i)
      	{
      		b.check_collision(*i);
      	}
      	// проверка столкновения пуль enemies со стенами лабиринта
      	for (auto &enemy : enemies)
      	{
      		auto enemy_bullets = enemy.get_bullets();
      		for (auto i = enemy_bullets.first; i != enemy_bullets.second; ++i)
      		{
      			b.check_collision(*i);
      		}
      	}
	}

	// выводит изменения
	void output(void)
	{
		window.clear();
		b.draw(window, clock.getElapsedTime().asMilliseconds());

		// вычисляем координаты мыши
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f globalMousePos = window.mapPixelToCoords(mousePos);

		if (is_person_info_panel(globalMousePos))	// если мышь наведена на персонажа
		{
			show_person_info_panel();// выводим информацию о персонаже
		}
		else
		{
			show_shop_info_pannel();// иначе, выводим магазин
		}
		for (auto &enemy : enemies)
		{
			enemy.draw(window, clock.getElapsedTime().asMilliseconds());
		}
		h.draw(window, clock.getElapsedTime().asMilliseconds());
		// Отображаем содержимое окна на экране
        window.display();
	}

	// изменяет параметры
	void new_level(void)
	{
		// увеличиваем счетчик текущего уровня
		++cur_level;
		// увеличиваем размер поля
		++board_width; ++board_height;
		// чистим список врагов
		enemies.clear();
	}

	// по параметрам генерируем уровень
	void init(void)
	{
		// устанавливаем board_width клеток в ширину, board_height в высоту
		b.set_board_size(board_width, board_height);
		// генерируем лабиринт
		b.build();

		// обновляем размеры ячейки и получаем рекомендованный размер персонажа
		people_size = b.get_people_size();

		sf::Vector2f cell_size = b.get_cell_size();
		cell_w = cell_size.x; cell_h = cell_size.y;

		// расставляем врагов, так чтобы они находились в уникальных позициях

		// ИНИЦИАЛИЗИРУЕМ ГЕНЕРАТОРО СЛУЧАЙНЫХ ЧИСЕЛ
		// Создание объекта случайного устройства для получения истинно случайного начального значения
	    std::random_device rd;
	    // Создание объекта генератора случайных чисел с начальным значением от случайного устройства
	    std::mt19937 gen(rd());
	    // Создание объекта равномерного распределения от 0 до 2^16
	    std::uniform_int_distribution<> dis(0, 1 << 16);
	    // ЗАКОНЧИЛИ ИНИЦИАЛИЗИРОВАТЬ ГЕНЕРАТОР

	    // расставляем врагов
	    int cur_row, cur_col;
		std::set<std::pair<int, int>> enemies_positions; // сет со стартовыми позициями enemy
		// количество врагов будет равно max(board_width, board_height)
		for (std::size_t i = 0; i < std::max(board_width, board_height); ++i)
		{
			cur_row = dis(gen) % board_height;
			cur_col = dis(gen) % board_width;
			// до тех пор пока координата не уникальна продолжаем генерировать
			// также нельзя чтобы враг стоял в тойже ячейке что и hero
			while (0 != enemies_positions.count(std::make_pair(cur_row, cur_col)) || cur_row + cur_col == 0)
			{
				cur_row = dis(gen) % board_height;
				cur_col = dis(gen) % board_width;
			}
			// запоминаем новую уникальную позицию
			enemies_positions.insert(std::make_pair(cur_row, cur_col));
			// генерируем тип enemy
			int cur_num_model = dis(gen) % num_models_of_enemy;
			// вычисляем позицию на карте
			sf::FloatRect cur_enemy_pos(
				cur_col * cell_w + cell_w / 2 - people_size / 2,
				cur_row * cell_h + cell_h / 2 - people_size / 2,
				people_size,
				people_size
				);

			// заносим enemy в enemies
			enemies.emplace_back(cur_num_model);
			// устанавливаем в нужную позицию
			enemies[i].set_position(cur_enemy_pos);

			// в зависимости от типа enemy устанавливаем оружие и спрайт
			switch (cur_num_model)
			{
			case 0:
				enemies[i].set_weapon(AKR(people_size * 1 / 4, people_size * 1 / 2, people_size * 1 / 2));
				break;
			}
		}

		// устанавливаем hero 
		h.set_position(sf::FloatRect(
			cell_w / 2 - people_size / 2,
			cell_h / 2 - people_size / 2,
			people_size, 
			people_size)
		);

		h.set_weapon(AKR(people_size / 4, people_size / 2, people_size / 2));
		h.add_bomb();
	}	

	void make_board_boom(void)
	{
		// получаем позиции бомбы на карте
		std::vector<sf::Vector2f> bombs_pos = h.boom();
		// для каждой позиции
		for (auto bomb_pos : bombs_pos)
		{
			// запишем это в виде sf::FloatRect
			sf::FloatRect bomb_rect(bomb_pos.x, bomb_pos.y, 0, 0);
			// получим список клеток, которые подпадают под взрыв
			std::vector<std::pair<int, int>> cell_cors = b.neighbour(bomb_rect);
			//считаем ячейку в которой находится hero
			sf::Vector2i hero_cor = b.calc_pos_on_board(h.get_hitbox());
			// просматриваем всех персонажей и проверяем принадлежность этим ячейкам
			for (auto cor : cell_cors)
			{
				for (auto &enemy : enemies)
				{
					// вычисляем координату enemy на доске
					sf::Vector2i enemy_cor = b.calc_pos_on_board(enemy.get_hitbox());
					if (cor.first == enemy_cor.x && cor.second == enemy_cor.y)
					{
						enemy.change_cur_health(-Bomb::get_damage());// уменьшаем здоровье на урон бомбы
					}
				}
				// делаем тоже самое для hero
				if (cor.first == hero_cor.x && cor.second == hero_cor.y)
				{
					h.change_cur_health(-Bomb::get_damage());
				}
				// запускаем у текцщей ячейки анимацию взрыва
				b.boom_cell(cor.first, cor.second, clock.getElapsedTime().asMilliseconds());
			}
			// взорвем ближайшую стену
			b.wall_destroyer(bomb_rect);
		}
	}

	// работа с боковой панелью информации (возращает true если, вывело информацию false иначе)
	bool show_person_info_panel(void)
	{
		// получаем координаты мыши на экране
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f globalMousePos = window.mapPixelToCoords(mousePos);

        // ищем персонажа на которого наведена мышь
        for (auto &enemy : enemies)
        {
        	// произошло пересечение
        	if (enemy.intersection(globalMousePos))
        	{
        		__int64_t cur_time = clock.getElapsedTime().asMilliseconds();
        		enemy.start_presentation_animation(cur_time);
        		// Заголовок (что мы смоотрим характеристики персонажа)
        		title1_text.setString("Character characteristics");
        		title1_text.setPosition(info_pannel_pos.left, 
        									info_pannel_pos.top);
        		// вычисляем его границы, чтобы знать куда размещять следующий
        		sf::FloatRect last_obj_pos = title1_text.getGlobalBounds();
        		// получаем спрайт для презентации
        		sf::Sprite present_sprite_people = enemy.get_presentation_sprite(cur_time);
        		// размещаем его
        		present_sprite_people.setOrigin(0, 0);
        		present_sprite_people.setPosition(last_obj_pos.left,
        									last_obj_pos.top + last_obj_pos.height);
        		// вычисляем позицию картинки на доске
        		last_obj_pos = present_sprite_people.getGlobalBounds();
        		// получаем информацию о enemy
        		std::string people_info_string = "\tHealth:\t" + std::to_string(std::max(0, enemy.get_cur_health())) + "\n" +
        							 			 "\tSpeed:\t"  + std::to_string(enemy.get_speed() / 10) + "\n";
        		// вставляем ее в sf::Text для представления
        		people_info_text.setString(people_info_string);
        		// позиционируем ее в info_panel
        		people_info_text.setPosition(last_obj_pos.left,
        									last_obj_pos.top + last_obj_pos.height);
        		// вычисляем позицию предыдущего элемента
        		last_obj_pos = people_info_text.getGlobalBounds();

        		// Заголовок (что мы смоотрим характеристики оружия)
        		title2_text.setString("Gun characteristics");
        		title2_text.setPosition(last_obj_pos.left,
        									last_obj_pos.top + last_obj_pos.height);
        		last_obj_pos = title2_text.getGlobalBounds();
        		// получаем спрайт презентации оружия
        		sf::Sprite present_sprite_gun = enemy.get_gun_presentation_sprite(cur_time);
        		present_sprite_gun.setOrigin(0, 0);
        		// устанавливаем его в нужную позицию
        		present_sprite_gun.setPosition(last_obj_pos.left,
        										last_obj_pos.top + last_obj_pos.height);
        		// обновляем позицию картинки на доске
        		last_obj_pos = present_sprite_gun.getGlobalBounds();
        		// получаем информацию о оружии
        		std::string gun_info_str = "\tBullets:\t" + std::to_string(enemy.get_magazine_info().first) + "/" + std::to_string(enemy.get_magazine_info().second) + "\n" +
        									"\tDamage:\t" + std::to_string(enemy.get_damage()) + "\n" +
        									"\tSpread:\t" + std::to_string(static_cast<int>(enemy.get_spread())) + "deg\n" +
        									"\tMagazines:\t" + std::to_string(enemy.get_gun_info()) + "\n" +
        									"\tRecharge time:\t" + std::to_string(enemy.get_recharge_time() / 1000) + "sec.";
        		gun_info_text.setString(gun_info_str);
        		gun_info_text.setPosition(last_obj_pos.left,
        										last_obj_pos.top + last_obj_pos.height);
        		window.draw(title1_text);
        		window.draw(present_sprite_people);
        		window.draw(people_info_text);

        		window.draw(title2_text);
        		window.draw(present_sprite_gun);
        		window.draw(gun_info_text);
        		return true;
        	}
        }

        if (h.intersection(globalMousePos))
    	{
    		__int64_t cur_time = clock.getElapsedTime().asMilliseconds();
    		h.start_presentation_animation(cur_time);
    		// получаем спрайт для презентации
    		sf::Sprite present_sprite_people = h.get_presentation_sprite(cur_time);
    		// размещаем его
    		// вычисляем позицию картинки на доске
    		sf::FloatRect last_obj_pos(present_sprite_people.getGlobalBounds());
    		present_sprite_people.setPosition(info_pannel_pos.left + last_obj_pos.width / 2, 
    									info_pannel_pos.top + last_obj_pos.height / 2);
    		// обновляем позицию картинки на доске
    		last_obj_pos = present_sprite_people.getGlobalBounds();
    		// получаем информацию о h
    		std::string people_info_string = "\tHealth:\t" + std::to_string(std::max(0, h.get_cur_health())) + "\n" +
    							 			 "\tSpeed:\t"  + std::to_string(h.get_speed() / 10) + "\n" +
    							 			 "\tBombs:\t"  + std::to_string(h.get_bomb_cnt()) + "\n" +
    							 			 "\tBomb damage:\t" + std::to_string(Bomb::get_damage()) + "\n";
    		// вставляем ее в sf::Text для представления
    		people_info_text.setString(people_info_string);
    		// позиционируем ее в info_panel
    		people_info_text.setPosition(last_obj_pos.left,
    									last_obj_pos.top + last_obj_pos.height);
    		// вычисляем позицию предыдущего элемента
    		last_obj_pos = people_info_text.getGlobalBounds();
    		// получаем спрайт презентации оружия
    		sf::Sprite present_sprite_gun = h.get_gun_presentation_sprite(cur_time);
    		present_sprite_gun.setOrigin(0, 0);
    		// устанавливаем его в нужную позицию
    		present_sprite_gun.setPosition(last_obj_pos.left,
    										last_obj_pos.top + last_obj_pos.height);
    		// обновляем позицию картинки на доске
    		last_obj_pos = present_sprite_gun.getGlobalBounds();
    		// получаем информацию о оружии
    		std::string gun_info_str = "\tBullets:\t" + std::to_string(h.get_magazine_info().first) + "/" + std::to_string(h.get_magazine_info().second) + "\n" +
    									"\tDamage:\t" + std::to_string(h.get_damage()) + "\n" +
    									"\tSpread:\t" + std::to_string(static_cast<int>(h.get_spread())) + "deg\n" +
    									"\tMagazines:\t" + std::to_string(h.get_gun_info()) + "\n" +
    									"\tRecharge time:\t" + std::to_string(h.get_recharge_time() / 1000) + "sec.";
    		gun_info_text.setString(gun_info_str);
    		gun_info_text.setPosition(last_obj_pos.left,
    										last_obj_pos.top + last_obj_pos.height);
    		window.draw(present_sprite_people);
    		window.draw(people_info_text);
    		window.draw(present_sprite_gun);
    		window.draw(gun_info_text);
    		return true;
    	}
        return false;
	}

	// Рисует shop
	bool show_shop_info_pannel(void)
	{
		// отображаем cash
		cash_text.setString("\t\tYour balance:\t" + std::to_string(h.get_cash()));
		window.draw(cash_text);

		// получаем координаты мыши
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f globalMousePos = window.mapPixelToCoords(mousePos);

		bomb_btn.draw(window, globalMousePos);
		magazine_btn.draw(window, globalMousePos);
		recharge_btn.draw(window, globalMousePos);
		flame_arrester_btn.draw(window, globalMousePos);
		first_aid_kit_btn.draw(window, globalMousePos);
		armor_btn.draw(window, globalMousePos);
		speed_btn.draw(window, globalMousePos);
		menu_btn.draw(window, globalMousePos);
		return true;
	}

	// настраивает sf::Text под единый стиль
	void settings_text(sf::Text& text, const sf::Color& color=sf::Color::White)
	{
		text.setFont(font);
		text.setCharacterSize(20);
	    text.setFillColor(color);
	}

	// инициализирует shop info panel
	void button_init(void)
	{
		// резервируем место для вывода cash 
		cash_text.setPosition(info_pannel_pos.left, 15.0f);
		// позиционируем кнопки
		sf::FloatRect last_obj_pos(info_pannel_pos.left, 0.0f, info_pannel_pos.width, 50.0f);
		bomb_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Bomb:\t" + std::to_string(costs::BOMB));
		last_obj_pos = bomb_btn.get_hitbox();

		magazine_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Magazine:\t" + std::to_string(costs::MAGAZINE));
		last_obj_pos = magazine_btn.get_hitbox();

		recharge_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Recharge:\t" + std::to_string(costs::RECHARGE));
		last_obj_pos = recharge_btn.get_hitbox();

		flame_arrester_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Flame arrester:\t" + std::to_string(costs::FLAME_ARRESTER));
		last_obj_pos = flame_arrester_btn.get_hitbox();

		first_aid_kit_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "First aid kit:\t" + std::to_string(costs::FIRS_AID_KIT));
		last_obj_pos = first_aid_kit_btn.get_hitbox();

		armor_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Armor:\t" + std::to_string(costs::ARMOR));
		last_obj_pos = armor_btn.get_hitbox();

		speed_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 15.0f, last_obj_pos.width, last_obj_pos.height), "Speed:\t" + std::to_string(costs::SPEED));
		last_obj_pos = speed_btn.get_hitbox();

		menu_btn = Button(sf::FloatRect(last_obj_pos.left, last_obj_pos.top + last_obj_pos.height + 190.0f, last_obj_pos.width, last_obj_pos.height), "Menu");
		last_obj_pos = menu_btn.get_hitbox();
	}

	// проверяет наведена ли в данный момент мышка на какого либо персонажа
	bool is_person_info_panel(const sf::Vector2f& globalMousePos)
	{
        // ищем персонажа на которого наведена мышь
        for (auto &enemy : enemies)
        {
        	// произошло пересечение
        	if (enemy.intersection(globalMousePos))
        	{
        		return true;
        	}
        }
        if (h.intersection(globalMousePos))
    	{
    		return true;
    	}
    	// если такого не нашли возращаем false
        return false;
	}


	sf::RenderWindow &window;	// окно в котором будем это отображать
	std::size_t board_width, board_height, cur_level, num_models_of_enemy;	// переменные для игры
	enum costs			// цены на товары
	{
		BOMB = 5,
		MAGAZINE = 3,
		RECHARGE = 1,
		FLAME_ARRESTER = 2,	// уменьшает разброс
		FIRS_AID_KIT = 3,
		ARMOR = 3,
		SPEED = 2
	};
	float cell_w, cell_h, people_size;		// информация о доске
	bool mouse_button_is_pressed;       // зажата ли клавиша мыши используется для стрельбы (удержание => очередь)
	float move_direct_x, move_direct_y; // направление ходьбы персонажа
	sf::FloatRect info_pannel_pos;	// позиция боковой панели с информацией
	Board b;	// объект доски
	Hero h;		// объект персонажа
	Button bomb_btn, magazine_btn, recharge_btn, flame_arrester_btn, first_aid_kit_btn, armor_btn, speed_btn, menu_btn; // кнопки для управления игрой
	sf::Font font;	// шрифт для вывод текста
	sf::Text people_info_text, gun_info_text, title1_text, title2_text, cash_text;	// sf::Text для вывода информации об игре
	__int64_t last_hero_moving_time;	// нужно чтобы откатываться назад при столкновении
	std::vector<Enemy> enemies;			// массив с объектами врагов
	sf::Clock clock;					// таймер
};

int main()
{
	    // Создаем объект окна
	sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Game", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

	GameManager g = GameManager(window);
	g.run();
}
