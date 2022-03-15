#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define FILE_NAME_CAL "CalDb.bin"
#define FILE_NAME_LIMIT "Limits.bin"

//ANSI COLOR
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m" //pink
#define CYAN    "\x1b[36m" //lite blue
#define RESET   "\x1b[0m"

#define MAX_NUMBER ((int32_t)1000000) //max number a user can enter (from positive to negative)

typedef struct {
	int32_t cals; //also weight
	uint16_t year;
	uint8_t month;
	uint8_t day;
	bool inUse;
	bool weighIn;
} dayItem;

dayItem allItems[UINT8_MAX];
uint16_t year = 0;
uint8_t month = 0;
uint8_t day = 0;

typedef struct {
	int32_t calLimit;

	//walking timer
	bool timerActive;
	int64_t startTime;
	double timerMultiplier;
	int32_t calsPerMin;

} limits;
limits allLimits;

//these are updated by function
time_t rawtime; //assuming this is posix time
struct tm* timeinfo;

//updated by DrawSetDay, for display purpose
int32_t dispLossSum = 0;
int32_t dispGainSum = 0;
int32_t dispTimeSum = 0;


void ResetTimer(void) {
	allLimits.timerActive = false;
	allLimits.startTime = 0;
}

void InitAllLimits(void) {
	puts(YELLOW"Initializing Limits\n"RESET);
	allLimits.calLimit = 1250;

	ResetTimer();
	allLimits.timerMultiplier = 0.9;
	allLimits.calsPerMin = -4;
}

void InitDayItem(void) {
	puts(YELLOW"Clearing Items\n"RESET);
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		allItems[i].inUse = false;
	}
}

void SortdayItem(void) {
	//bubble sort the inuse
	bool madeChanges = false;
	do {
		madeChanges = false;
		for (uint8_t i = 0; i < UINT8_MAX - 1; ++i) {
			if (!allItems[i].inUse && allItems[i + 1].inUse) {
				const dayItem tmp = allItems[i];
				allItems[i] = allItems[i + 1];
				allItems[i + 1] = tmp;
				madeChanges = true;
			}
		}
	} while (madeChanges);
}

bool RemoveItemsNotFromToday(void) {
	//removed items not from today
	//weighIn are kept
	bool resetItem = false;
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		if (allItems[i].inUse
			&& !allItems[i].weighIn
			&& (allItems[i].day != day || allItems[i].month != month || allItems[i].year != year)
			)
		{
			allItems[i].inUse = false;
			resetItem = true;
		}
	}
	
	//if you had to use the reset
	if (resetItem) { 
		ResetTimer(); //reset timers left on
		SortdayItem(); //fill in gaps made by remove
	}
	
	return resetItem; //sends back if it had to reset the day
}

void DeleteItem(const uint8_t index) {
	allItems[index].inUse = false;
	SortdayItem();
}

void Save(void) {
	FILE* file = NULL;
	bool errorCheck = false;

	file = fopen(FILE_NAME_CAL, "wb");
	if (NULL == file) {
		puts(RED"Failed to save "RESET FILE_NAME_CAL);
	}
	else {
		errorCheck = fwrite(allItems, sizeof(dayItem), UINT8_MAX, file) != UINT8_MAX - 1;
		if (0 == errorCheck) {
			puts(RED"Failed to save "RESET FILE_NAME_CAL);
		}
		else {
			fclose(file);
			file = NULL;
			puts(GREEN"Saved " RESET FILE_NAME_CAL);
		}
	}

	//save limit
	file = fopen(FILE_NAME_LIMIT, "wb");
	if (NULL == file) {
		puts(RED"Failed to save "RESET FILE_NAME_LIMIT);
	}
	else {
		errorCheck = fwrite(&allLimits, 1, sizeof(limits), file) != sizeof(limits) - 1;
		if (0 == errorCheck) {
			puts(RED"Failed to save "RESET FILE_NAME_LIMIT);
		}
		else {
			fclose(file);
			file = NULL;
			puts(GREEN"Saved " RESET FILE_NAME_LIMIT);
		}
	}

}

void Load(void) {

	FILE* file = NULL;
	bool errorCheck = false;

	if ((file = fopen(FILE_NAME_LIMIT, "rb")) == NULL) {
		puts(YELLOW "Not able to open: " RESET FILE_NAME_LIMIT RESET);
		InitAllLimits();
	}
	else {
		errorCheck = fread(&allLimits, 1, sizeof(limits), file) != sizeof(limits) - 1;
		if (0 == errorCheck) {
			puts(RED "Not able to load file: " RESET FILE_NAME_LIMIT);
		}
		else {
			fclose(file);
			file = NULL;
			puts(GREEN"File loaded " RESET FILE_NAME_LIMIT);
		}
	}

	if ((file = fopen(FILE_NAME_CAL, "rb")) == NULL) {
		puts(YELLOW "Not able to open " RESET FILE_NAME_CAL YELLOW " Making new save file" RESET);
		InitDayItem();
	}
	else {
		errorCheck = fread(allItems, sizeof(dayItem), UINT8_MAX, file) != UINT8_MAX - 1;
		if (0 == errorCheck) {
			puts(RED "Not able to load file: " RESET FILE_NAME_CAL);
		}
		else {
			fclose(file);
			file = NULL;
			puts(GREEN"File loaded " RESET FILE_NAME_CAL);
		}
	}



}

void DrawDate(void) {
	printf(CYAN"Date"RESET" %02d-%02d-%04d\n", month, day, year);
}

void UpdateTimeinfoToRTC(void) {
	time(&rawtime);
	timeinfo = localtime(&rawtime);
}

void SetTimeToToday() {
	UpdateTimeinfoToRTC();

	day = (uint8_t)timeinfo->tm_mday;
	month = (uint8_t)timeinfo->tm_mon + 1;
	year = (uint16_t)timeinfo->tm_year + 1900;
}

double ReadDouble(void) {
	while (true) {
		//get new item
		double num;
		const int32_t nitems = scanf("%lf", &num);
		//clean input
		int c;
		while ((c = getchar()) != '\n' && c != EOF);
		//errors
		if (nitems == EOF || nitems == 0) {
			puts(RED"error"RESET", numbers only.");
			continue;
		}
		if (num > MAX_NUMBER || num < -MAX_NUMBER) {
			printf(RED"error"RESET", no numbers greater or smaller than %d.\n", MAX_NUMBER);
			continue;
		}
		return num;
	}
}

int32_t ReadInt(void) {
	return (int32_t)ReadDouble();
}

int32_t CalsToSecs(const int32_t cals) {
	const int32_t tmp = -(int32_t)(-cals / -allLimits.calsPerMin / allLimits.timerMultiplier);
	return tmp < 0 ? -tmp : tmp;
}

void DrawSetDay(const bool weighIn) {
	//data upkeep
	SetTimeToToday();
	RemoveItemsNotFromToday();

	if (!weighIn) {
		puts(CYAN"\nAll items for this date"RESET);
		DrawDate();
	}
	else {
		printf(YELLOW"\nWeight progress all time\n"RESET);
	}

	int32_t sum = 0; //sum up cals
	int32_t lossSum = 0; //holds negative entries
	int32_t gainSum = 0; //postive
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		if (allItems[i].inUse
			&& ((allItems[i].weighIn == true && weighIn)
				|| (allItems[i].weighIn == false && !weighIn
					&& allItems[i].day == day
					&& allItems[i].month == month
					&& allItems[i].year == year))) {

			const int32_t tmpCals = allItems[i].cals;
			if (weighIn) {
				printf(CYAN "Date"RESET" %02d-%02d-%04d ", allItems[i].month, allItems[i].day, allItems[i].year);
			}
			else {
				sum += tmpCals;
				if (tmpCals < 0) {
					lossSum += tmpCals;
				}
				else {
					gainSum += tmpCals;
				}
			}
			printf(CYAN "id: %03d"YELLOW" amount:"RESET" %04d", i, allItems[i].cals);
			if (tmpCals < 0)
			{
				const int32_t tmpTime = CalsToSecs(tmpCals);
				printf(YELLOW" time:"RESET" %02d:%02d", tmpTime / 60, tmpTime % 60);
			}
			printf("\n");
		}
	}

	//update for display values
	dispLossSum = lossSum;
	dispGainSum = gainSum;
	dispTimeSum = CalsToSecs(lossSum);

	if (!weighIn) {
		const int32_t caloriesLeft = allLimits.calLimit - sum;
		printf(YELLOW"\nCalories Left:"RESET);

		if (caloriesLeft < 0) {
			printf(RED " %d" RESET, caloriesLeft);
		}
		else {
			printf(" %d", caloriesLeft);
		}

		printf("\n");

		if (caloriesLeft < 0) {
			int32_t tmpTimeLeft = CalsToSecs(caloriesLeft);
			if (tmpTimeLeft <= 0) tmpTimeLeft = 1;
			printf(YELLOW"Walk time needed:"RED" %02d:%02d\n"RESET, tmpTimeLeft / 60, tmpTimeLeft % 60);
		}
	}
	else {
		printf("\n");
	}
}

void InitItem(const uint8_t index) {
	SetTimeToToday();
	allItems[index].inUse = true;
	allItems[index].day = day;
	allItems[index].month = month;
	allItems[index].year = year;
	allItems[index].cals = 0;
	allItems[index].weighIn = false;
}

void EditItem(const uint8_t index, const bool askServings) {
	//setting new item
	if (allItems[index].weighIn) { //enter weight
		puts("Weight?");
		allItems[index].cals = ReadInt();
	}
	else { //or enter cals
		printf("Calories? Old value was: %d \n", allItems[index].cals);
		allItems[index].cals = ReadInt();
		if (askServings) {
			puts("servings?");
			allItems[index].cals = (int32_t)((double)allItems[index].cals * ReadDouble());
		}
	}
	//ending items
	Save();
	DrawSetDay(allItems[index].weighIn);
}

uint8_t FineFreeItemId(void) {
	for (uint8_t i = 0; i < UINT8_MAX; ++i) {
		if (!allItems[i].inUse) {
			InitItem(i);
			return i;
		}
	}
	puts(RED"ERROR:"RESET" Out of free Items, please start new save by removing " FILE_NAME_CAL);
	exit(EXIT_FAILURE);
}

uint8_t AddNewItem(const bool weighIn, const bool askServings) {
	const uint8_t tmpItemId = FineFreeItemId();
	allItems[tmpItemId].weighIn = weighIn;
	EditItem(tmpItemId, askServings);
}

void AddNewCal(const int32_t cals) {
	allItems[FineFreeItemId()].cals = cals;
}

void StartTimer(void) {
	UpdateTimeinfoToRTC();
	allLimits.timerActive = true;
	allLimits.startTime = (int64_t)rawtime;
	Save();
}

int32_t GetTimerRuntimeMins(void) {
	UpdateTimeinfoToRTC();

	if (allLimits.timerActive) {
		return (int32_t)((int64_t)rawtime - allLimits.startTime) / 60;
	}
	return 0;
}

int32_t GetTimerCals(void) {
	if (allLimits.timerActive) {
		return (int32_t)(GetTimerRuntimeMins() * allLimits.timerMultiplier * allLimits.calsPerMin);
	}
	return 0;
}

void DrawTimerStat(void) {
	if (allLimits.timerActive) {
		printf(YELLOW"Timer "GREEN"on"YELLOW" for"RESET" %02d:%02d"YELLOW" for"RESET" %d "YELLOW"cals."RESET"\n", GetTimerRuntimeMins() / 60, GetTimerRuntimeMins() % 60, GetTimerCals());
	}
	else {
		puts(YELLOW"Timer "RED"not"YELLOW" active."RESET);
	}
}

void EndTimerAndAddCals(void) {
	AddNewCal(GetTimerCals());
	ResetTimer();
	Save();
}

void DrawLimits(void) {
	printf(CYAN"Calorie limit"RESET" %d\n"CYAN"Calories Multiplier"RESET" %.2f\n"CYAN"Calories Per Minute"RESET" %d\n", allLimits.calLimit, allLimits.timerMultiplier, allLimits.calsPerMin);
}

void EditLimits(void) {
	printf(CYAN"Set Calorie limit, old was"RESET" %d \n", allLimits.calLimit);
	allLimits.calLimit = ReadInt();
	if (allLimits.calLimit == 0) allLimits.calLimit = 1;

	printf(CYAN"Set Walking Calories Multiplier, old was"RESET" %.2f \n", allLimits.timerMultiplier);
	allLimits.timerMultiplier = ReadDouble();
	if (allLimits.timerMultiplier == 0) allLimits.timerMultiplier = 1;

	printf(CYAN"Set Walking Calories Per Minute, old was"RESET" %d \n", allLimits.calsPerMin);
	allLimits.calsPerMin = ReadInt();
	if (allLimits.calsPerMin == 0) allLimits.calsPerMin = -1;
	if (allLimits.calsPerMin > 0) allLimits.calsPerMin = -allLimits.calsPerMin;

	Save();
}

void MainMenu(void) {
	while (true) {
		//draw menu
		//loop/data upkeep
		static bool DrawMenuText = true; //skips drawing menu for only the next loop

		//limited menu
		if (DrawMenuText) {
			DrawTimerStat();
			puts("1."CYAN"Add"RESET" 2."CYAN"Edit"RESET" 3."CYAN"Refresh"RESET" 7."CYAN"More"RESET" 8."CYAN"Timer"RESET" 13."CYAN"Exit"RESET);
		}

		DrawMenuText = true;

		//clean up data
		if (RemoveItemsNotFromToday()){
			Save(); //save if it had to reset
		}

		//decode choice
		const int32_t menu = ReadInt();
		switch (menu)
		{
		case 0: //free spot

			break;

		case 1: //New food item
			AddNewItem(false, true);
			break;

		case 2: //edit food item
			puts("Enter Food Item Id:");
			EditItem((uint8_t)ReadInt(), true);
			break;

		case 3: //draw food items
			DrawSetDay(false);
			break;

		case 4: //add weigth
			AddNewItem(true, false);
			break;

		case 5: //edit weigh
			puts("Enter weight Item Id:");
			EditItem((uint8_t)ReadInt(), false);
			break;

		case 6: //display weight
			DrawSetDay(true);
			break;

		case 7: //shows more options
			#define FOOD_ITEM_STRING CYAN"Food:"RESET"   1.New 2.Edit 3.List\n"
			#define WEIGHT_STRING    CYAN"Weight:"RESET" 4.New 5.Edit 6.List\n"
			#define TIME_STRING      CYAN"Etc:"RESET"    7.List all options 8.Toggle timer\n"
			#define LIMIT_STRING     CYAN"Limits:"RESET" 11.Edit 12.List\n"
			#define SAVEQUITE_STRING CYAN"Other:"RESET"  13.Save and Quit 14.Save 16.Only Quit\n"
			#define DELETE_STRING    CYAN"Delete"RESET"  09.Delete 17.Delete All Entries And Weight"
			puts("\n" FOOD_ITEM_STRING WEIGHT_STRING TIME_STRING LIMIT_STRING SAVEQUITE_STRING DELETE_STRING);
			printf(CYAN"Totals:"RESET" "YELLOW"Calories"RESET" %d "YELLOW"burned"RESET" %d "YELLOW"time"RESET" %02d:%02d \n", dispGainSum, dispLossSum, dispTimeSum / 60, dispTimeSum % 60);
			DrawMenuText = false;
			break;

		case 8://toggle timer
			if (!allLimits.timerActive) {
				StartTimer();
			}
			else {
				EndTimerAndAddCals();
			}
			DrawSetDay(false);
			break;

		case 9:
			puts("Enter Food Item Id to "RED"delete:"RESET);
			uint8_t id = (uint8_t)ReadInt();
			printf(RED"Are you sure you want to delete %d?"RESET"\n1.Yes 0.No\n", id);
			if (ReadInt() == 1) {
				DeleteItem(id);
				Save();
			}
			DrawSetDay(false);
			break;

		case 11: //set cal limit
			EditLimits();
			break;

		case 12: //draw limit
			DrawLimits();
			break;

		case 13: //save and quit
			Save();
			return;

		case 14: //save
			Save();
			DrawSetDay(false);
			break;

		case 16: //only exit
			return;
			
		case 17: //delete all entries
			puts(RED"Are you sure you want to delete all entries and weight?"RESET"\n1.Yes 0.No");
			if (ReadInt() == 1) {
				InitDayItem();
				Save();
			}
			DrawSetDay(false);
			break;

		default:
			AddNewCal(menu);
			Save();
			DrawSetDay(false);
			break;
		}//end of case
	}//end of menu loop
}

int main(void) {

	//init
	Load();
	SetTimeToToday();
	DrawSetDay(false);

	//main app
	MainMenu();

	//exit
	puts(GREEN "Exit, no errors" RESET);
	return EXIT_SUCCESS;

}
