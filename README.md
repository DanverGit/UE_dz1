# Домашнее задание №1: основы механики

Проект Unreal Engine **5.8.2**, C++, Third Person Template. Объём работы соответствует основным пунктам задания на **8 баллов**.

## Запуск

1. Установить Unreal Engine 5.8 и Visual Studio с поддержкой C++ для Unreal Engine.
2. Открыть `dz1/dz1.uproject`, согласиться на сборку модулей (либо собрать `dz1Editor`, Development Editor, Win64).
3. Открывается `Content/ThirdPerson/Lvl_ThirdPerson`. Нажать Play и щёлкнуть по окну игры.

## Управление

| Клавиша | Действие |
| --- | --- |
| W / A / S / D | Перемещение |
| Мышь | Поворот камеры |
| Пробел | Прыжок |
| Left Shift, удерживать | Спринт: скорость 900 вместо 500 см/с |
| E | Взаимодействие: `Interaction triggered!` |
| Esc | Остановить игру в редакторе |

## Реализация

- `AHomeworkCharacter : ACharacter` является родителем стандартного `BP_ThirdPersonCharacter`. Камера, движение и прыжки сохранены из Third Person Template.
- `IA_Sprint` и `IA_Interact` добавлены в `Input/IMC_Default`. Обработчики привязаны в C++ через `UEnhancedInputComponent::BindAction` в `SetupPlayerInputComponent`.
- Спринт обрабатывает `Started`, `Completed` и `Canceled`. Скорость возвращается к обычной при отпускании клавиши, отмене действия и потере управления персонажем.
- `bIsSprinting` доступен в Blueprint через `UPROPERTY(BlueprintReadOnly)`.
- `OnInteractionTriggered` - Dynamic Multicast Delegate. Нажатие E вызывает `Broadcast`; Blueprint подписывается на событие и выполняет `Print String` с текстом `Interaction triggered!`.
- Стандартный `ABP_Unarmed` получает флаг персонажа через C++-родителя `UHomeworkAnimInstance`. В машине состояний добавлено состояние `Sprint` со стандартной анимацией бега и переходами по флагу.
- Выносливость не входит в вариант задания на 8 баллов.

Исходники находятся в `dz1/Source/dz1`. Редакторский модуль `dz1Editor` содержит служебную команду подготовки ресурсов; он не включается в игровую сборку. Готовые `.uasset` и `.umap` хранятся в репозитории, запуск генерации для обычной работы не требуется.

## Проверка в редакторе

1. Проверить перемещение, вращение камеры и прыжок.
2. Двигаться с зажатым Left Shift: скорость возрастает, в отладке Animation Blueprint активно состояние Sprint.
3. Отпустить Shift: скорость 500, возврат к обычному состоянию движения.
4. Нажать E: появляется `Interaction triggered!`. Удержание E не должно повторять событие каждый кадр.
5. В `BP_ThirdPersonCharacter` открыть Event Graph и проверить подписку на делегат; в `ABP_Unarmed` проверить Sprint и правила переходов.

## Сдача

По условию требуется ссылка на **публичный GitHub/GitLab-репозиторий**. Публиковать нужно корень этой папки с README, `.gitignore` и папкой `dz1`. Генерируемые каталоги Binaries, DerivedDataCache, Intermediate, Saved исключены из Git.

## Выполненные проверки

- Сборка `dz1Editor Win64 Development` на UE 5.8.2: успешно.
- Компиляция BP_ThirdPersonCharacter, BP_ThirdPersonPlayerController, BP_ThirdPersonGameMode и ABP_Unarmed: успешно.
- Интеграционная проверка в тестовом игровом мире: создание персонажа из сохранённого Blueprint, привязки Enhanced Input, скорости 500/900, сброс по Completed/Canceled и UnPossess, подписка Blueprint на делегат и вызов обработчика, передача флага в AnimInstance.
- При симуляции движения машина анимаций действительно переключилась в `Sprint`, после отпускания - в `Walk / Run`. Проверка завершилась с кодом 0, без ошибок и предупреждений.
- Проверка выполняется без графического окна; ручная визуальная проверка Play в редакторе остаётся полезной перед сдачей.

Повторить проверку после сборки редакторского модуля (подставить путь к установленному движку):

```powershell
& '<UE_5.8>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/dz1/dz1.uproject" -run=PrepareHomework -Verify -unattended -nop4 -nullrhi -nosound -ddc=InstalledNoZenLocalFallback
```

Опция `-Verify` только проверяет проект. Запуск без неё повторно подготавливает ресурсы задания. Опция `-Inspect` выводит структуру графов. Файловый кэш в этой команде позволяет выполнять проверку при недоступном локальном Zen Server.
