---
id: pattern-005
category: deploy
language: cpp
score: 50
tags: [deploy, cpp]
---

## 컨텍스트
파일: main.cpp (Edit 완료)

## 핵심 코드
```cpp
std::string PromptHeroSelection(const std::vector<std::string>& heroes) {
    if (heroes.empty()) {
        throw std::runtime_error("В этом VPK не обнаружено героев.");
    }

    std::cout << "Выберите героя для экспорта:\n";
    for (std::size_t index = 0; index < heroes.size(); ++index) {
        std::cout << "  " << (index + 1) << ". " << heroes[index] << "\n";
    }
    std::cout << "Введите номер: ";

    std::size_t choice = 0;
    if (!(std::cin >> choice) || choice == 0 || choice > heroes.size()) {
        throw std::runtime_error("Неверный выбор героя.");
    }
    return heroes[choice - 1];
}
```

## 태그
- deploy
- cpp