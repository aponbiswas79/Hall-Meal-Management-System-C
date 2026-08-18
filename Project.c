#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STUDENTS     100
#define MAX_MEALS        300
#define STUDENT_FILE     "students.txt"
#define MEAL_FILE        "meals.txt"
#define ADMIN_USERNAME   "admin"
#define ADMIN_PASSWORD   "admin123"

#define BREAKFAST  0
#define LUNCH      1
#define DINNER     2

typedef struct {
    char name[50];
    char studentID[20];
    char discipline[50];
    char roomNumber[10];
    char password[30];
} Student;

typedef struct {
    char studentID[20];
    char date[15];
    int  meal[3];
    int  deliveryStatus[3];
} MealRecord;

Student    students[MAX_STUDENTS];
MealRecord meals[MAX_MEALS];
int        studentCount = 0;
int        mealCount    = 0;

/* Shared display data (previously duplicated in several functions). */
static const char *MEAL_NAMES[3]        = {"Breakfast", "Lunch", "Dinner"};
static const char *MEAL_NAMES_PADDED[3] = {"Breakfast", "Lunch    ", "Dinner   "};
static const char *STATUS_LABEL[2]      = {"Pending", "Delivered"};

/* ---------- Forward declarations ---------- */
void clearInputBuffer(void);
int  readLine(char *buf, size_t bufSize);
int  readInt(int *out);
void printLine(void);
void printHeader(const char *title);
void pressEnterToContinue(void);
void loadStudents(void);
void saveStudents(void);
void loadMeals(void);
void saveMeals(void);
int  findStudentByID(const char *id);
int  findMealRecord(const char *studentID, const char *date);
void registerStudent(void);
int  studentLogin(void);
void printMealRecord(int idx);
void bookMeal(int studentIdx);
void updateMeal(int studentIdx);
void cancelMeal(int studentIdx);
void viewMyMeals(int studentIdx);
void studentMenu(int studentIdx);
int  adminLogin(void);
void viewAllStudents(void);
void viewAllMealOrders(void);
void viewTotalMealCount(void);
void viewDeliveryList(void);
void updateDeliveryStatus(void);
void viewDeliveryReport(void);
void managementMenu(void);

/* ---------- Utility / I-O helpers ---------- */

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/*
 * Safely reads one line of text into buf (capacity bufSize), stripping the
 * trailing newline. Fixes two classes of bugs present in the original code:
 *   1. fgets()'s return value was never checked, so on EOF/read failure the
 *      caller went on to use an uninitialized buffer (undefined behavior).
 *   2. Lines longer than the buffer left unread characters in stdin, which
 *      silently corrupted the *next* read.
 * Returns 1 on success, 0 if no input could be read (EOF).
 */
int readLine(char *buf, size_t bufSize) {
    if (bufSize == 0) return 0;

    if (fgets(buf, (int)bufSize, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (len == bufSize - 1) {
        /* Buffer filled without hitting '\n' -> line was longer than buf.
           Discard the remainder so it doesn't corrupt the next read. */
        clearInputBuffer();
    }
    return 1;
}

/*
 * Safely reads an integer from stdin. The original code called
 * scanf("%d", &choice) without checking the result, so on non-numeric input
 * "choice" was left indeterminate and later read (undefined behavior).
 * Returns 1 and stores the parsed value in *out on success; returns 0 (and
 * leaves *out untouched) if the input was not a valid integer.
 */
int readInt(int *out) {
    int value;
    int result = scanf("%d", &value);
    clearInputBuffer();
    if (result != 1) {
        return 0;
    }
    *out = value;
    return 1;
}

void printLine(void) {
    printf("----------------------------------------------\n");
}

void printHeader(const char *title) {
    printf("\n==============================================\n");
    printf("  %s\n", title);
    printf("==============================================\n");
}

void pressEnterToContinue(void) {
    printf("\nPress Enter to continue...");
    getchar();
}

/* ---------- Persistence ---------- */

void loadStudents(void) {
    FILE *fp = fopen(STUDENT_FILE, "rb");
    if (fp == NULL) return;

    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1 || count < 0 || count > MAX_STUDENTS) {
        printf("[WARNING] %s is missing or corrupted; starting with no student data.\n",
               STUDENT_FILE);
        fclose(fp);
        return;
    }

    size_t got = fread(students, sizeof(Student), (size_t)count, fp);
    fclose(fp);

    if ((int)got != count) {
        printf("[WARNING] %s is truncated; loaded %d of %d student records.\n",
               STUDENT_FILE, (int)got, count);
    }
    studentCount = (int)got;
}

void saveStudents(void) {
    FILE *fp = fopen(STUDENT_FILE, "wb");
    if (fp == NULL) {
        printf("[ERROR] Could not open %s for writing.\n", STUDENT_FILE);
        return;
    }

    int ok = 1;
    if (fwrite(&studentCount, sizeof(int), 1, fp) != 1) ok = 0;
    if (ok && studentCount > 0 &&
        fwrite(students, sizeof(Student), (size_t)studentCount, fp) != (size_t)studentCount) {
        ok = 0;
    }
    if (!ok) {
        printf("[ERROR] Failed to write all student data to %s.\n", STUDENT_FILE);
    }
    fclose(fp);
}

void loadMeals(void) {
    FILE *fp = fopen(MEAL_FILE, "rb");
    if (fp == NULL) return;

    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1 || count < 0 || count > MAX_MEALS) {
        printf("[WARNING] %s is missing or corrupted; starting with no meal data.\n",
               MEAL_FILE);
        fclose(fp);
        return;
    }

    size_t got = fread(meals, sizeof(MealRecord), (size_t)count, fp);
    fclose(fp);

    if ((int)got != count) {
        printf("[WARNING] %s is truncated; loaded %d of %d meal records.\n",
               MEAL_FILE, (int)got, count);
    }
    mealCount = (int)got;
}

void saveMeals(void) {
    FILE *fp = fopen(MEAL_FILE, "wb");
    if (fp == NULL) {
        printf("[ERROR] Could not open %s for writing.\n", MEAL_FILE);
        return;
    }

    int ok = 1;
    if (fwrite(&mealCount, sizeof(int), 1, fp) != 1) ok = 0;
    if (ok && mealCount > 0 &&
        fwrite(meals, sizeof(MealRecord), (size_t)mealCount, fp) != (size_t)mealCount) {
        ok = 0;
    }
    if (!ok) {
        printf("[ERROR] Failed to write all meal data to %s.\n", MEAL_FILE);
    }
    fclose(fp);
}

/* ---------- Lookups ---------- */

int findStudentByID(const char *id) {
    int i;
    for (i = 0; i < studentCount; i++) {
        if (strcmp(students[i].studentID, id) == 0)
            return i;
    }
    return -1;
}

int findMealRecord(const char *studentID, const char *date) {
    int i;
    for (i = 0; i < mealCount; i++) {
        if (strcmp(meals[i].studentID, studentID) == 0 &&
            strcmp(meals[i].date, date) == 0)
            return i;
    }
    return -1;
}

/* ---------- Registration / Login ---------- */

void registerStudent(void) {
    printHeader("Student Registration");

    if (studentCount >= MAX_STUDENTS) {
        printf("[ERROR] Maximum student capacity reached.\n");
        pressEnterToContinue();
        return;
    }

    Student newStudent;
    memset(&newStudent, 0, sizeof(newStudent)); /* avoid writing uninitialized bytes to disk */

    printf("Enter Student Name      : ");
    readLine(newStudent.name, sizeof(newStudent.name));
    if (strlen(newStudent.name) == 0) {
        printf("[ERROR] Name cannot be empty.\n");
        pressEnterToContinue();
        return;
    }

    printf("Enter Student ID        : ");
    readLine(newStudent.studentID, sizeof(newStudent.studentID));
    if (strlen(newStudent.studentID) == 0) {
        printf("[ERROR] Student ID cannot be empty.\n");
        pressEnterToContinue();
        return;
    }

    if (findStudentByID(newStudent.studentID) != -1) {
        printf("[ERROR] A student with ID '%s' already exists.\n",
               newStudent.studentID);
        pressEnterToContinue();
        return;
    }

    printf("Enter Discipline   : ");
    readLine(newStudent.discipline, sizeof(newStudent.discipline));

    printf("Enter Hall Room Number  : ");
    readLine(newStudent.roomNumber, sizeof(newStudent.roomNumber));

    printf("Create a Password       : ");
    readLine(newStudent.password, sizeof(newStudent.password));

    if (strlen(newStudent.password) < 4) {
        printf("[ERROR] Password must be at least 4 characters.\n");
        pressEnterToContinue();
        return;
    }

    char confirm[30];
    printf("Confirm Password        : ");
    readLine(confirm, sizeof(confirm));

    if (strcmp(newStudent.password, confirm) != 0) {
        printf("[ERROR] Passwords do not match. Registration cancelled.\n");
        pressEnterToContinue();
        return;
    }

    students[studentCount++] = newStudent;
    saveStudents();

    printf("\n[SUCCESS] Registration complete! Welcome, %s.\n",
           newStudent.name);
    pressEnterToContinue();
}

int studentLogin(void) {
    printHeader("Student Login");

    char id[20], password[30];

    printf("Enter Student ID  : ");
    readLine(id, sizeof(id));

    printf("Enter Password    : ");
    readLine(password, sizeof(password));

    int idx = findStudentByID(id);
    if (idx == -1) {
        printf("[ERROR] Student ID not found.\n");
        pressEnterToContinue();
        return -1;
    }

    if (strcmp(students[idx].password, password) != 0) {
        printf("[ERROR] Incorrect password.\n");
        pressEnterToContinue();
        return -1;
    }

    printf("\n[SUCCESS] Welcome back, %s!\n", students[idx].name);
    return idx;
}

/* ---------- Meal display ---------- */

void printMealRecord(int idx) {
    int i;

    printf("\n  Date       : %s\n", meals[idx].date);
    printf("  Student ID : %s\n", meals[idx].studentID);
    printLine();
    printf("  %-10s | %-8s | Delivery\n", "Meal", "Booked");
    printLine();
    for (i = 0; i < 3; i++) {
        printf("  %s | %-8s | %s\n",
               MEAL_NAMES_PADDED[i],
               meals[idx].meal[i] ? "Yes" : "No",
               meals[idx].meal[i] ? STATUS_LABEL[meals[idx].deliveryStatus[i]] : "-");
    }
}

/* ---------- Student meal operations ---------- */

void bookMeal(int studentIdx) {
    printHeader("Book Meal");

    char date[15];
    printf("Enter date (YYYY-MM-DD): ");
    readLine(date, sizeof(date));

    if (strlen(date) != 10) {
        printf("[ERROR] Invalid date format. Use YYYY-MM-DD.\n");
        pressEnterToContinue();
        return;
    }

    const char *sid = students[studentIdx].studentID;
    int mIdx = findMealRecord(sid, date);

    if (mIdx == -1) {
        if (mealCount >= MAX_MEALS) {
            printf("[ERROR] Meal record storage full.\n");
            pressEnterToContinue();
            return;
        }
        mIdx = mealCount++;
        memset(&meals[mIdx], 0, sizeof(MealRecord));
        strcpy(meals[mIdx].studentID, sid);
        strcpy(meals[mIdx].date, date);
    }

    int choice;
    printf("\nSelect meals to BOOK (1=Yes, 0=No):\n");

    printf("  Breakfast : ");
    if (!readInt(&choice)) choice = -1; /* invalid input -> keep current value */
    meals[mIdx].meal[BREAKFAST] = (choice == 1) ? 1 : meals[mIdx].meal[BREAKFAST];

    printf("  Lunch     : ");
    if (!readInt(&choice)) choice = -1;
    meals[mIdx].meal[LUNCH] = (choice == 1) ? 1 : meals[mIdx].meal[LUNCH];

    printf("  Dinner    : ");
    if (!readInt(&choice)) choice = -1;
    meals[mIdx].meal[DINNER] = (choice == 1) ? 1 : meals[mIdx].meal[DINNER];

    saveMeals();
    printf("\n[SUCCESS] Meal booking saved.\n");
    printMealRecord(mIdx);
    pressEnterToContinue();
}

void updateMeal(int studentIdx) {
    printHeader("Update Meal");

    char date[15];
    printf("Enter date to update (YYYY-MM-DD): ");
    readLine(date, sizeof(date));

    const char *sid = students[studentIdx].studentID;
    int mIdx = findMealRecord(sid, date);

    if (mIdx == -1) {
        printf("[ERROR] No booking found for date %s.\n", date);
        pressEnterToContinue();
        return;
    }

    printf("\nCurrent booking:\n");
    printMealRecord(mIdx);

    printf("\nEnter new choices (1=Book, 0=Cancel, -1=Keep current):\n");

    int choice;
    printf("  Breakfast : ");
    if (readInt(&choice)) {
        if (choice == 1) meals[mIdx].meal[BREAKFAST] = 1;
        else if (choice == 0) meals[mIdx].meal[BREAKFAST] = 0;
    }

    printf("  Lunch     : ");
    if (readInt(&choice)) {
        if (choice == 1) meals[mIdx].meal[LUNCH] = 1;
        else if (choice == 0) meals[mIdx].meal[LUNCH] = 0;
    }

    printf("  Dinner    : ");
    if (readInt(&choice)) {
        if (choice == 1) meals[mIdx].meal[DINNER] = 1;
        else if (choice == 0) meals[mIdx].meal[DINNER] = 0;
    }

    saveMeals();
    printf("\n[SUCCESS] Meal updated.\n");
    printMealRecord(mIdx);
    pressEnterToContinue();
}

void cancelMeal(int studentIdx) {
    printHeader("Cancel Meal");

    char date[15];
    printf("Enter date to cancel (YYYY-MM-DD): ");
    readLine(date, sizeof(date));

    const char *sid = students[studentIdx].studentID;
    int mIdx = findMealRecord(sid, date);

    if (mIdx == -1) {
        printf("[ERROR] No booking found for date %s.\n", date);
        pressEnterToContinue();
        return;
    }

    printf("\nThe following booking will be CANCELLED:\n");
    printMealRecord(mIdx);
    printf("\nAre you sure? (1=Yes, 0=No): ");

    int confirm;
    if (!readInt(&confirm)) confirm = 0;

    if (confirm == 1) {
        meals[mIdx].meal[BREAKFAST]           = 0;
        meals[mIdx].meal[LUNCH]               = 0;
        meals[mIdx].meal[DINNER]              = 0;
        meals[mIdx].deliveryStatus[BREAKFAST] = 0;
        meals[mIdx].deliveryStatus[LUNCH]     = 0;
        meals[mIdx].deliveryStatus[DINNER]    = 0;

        saveMeals();
        printf("[SUCCESS] All meals cancelled for %s.\n", date);
    } else {
        printf("Cancellation aborted.\n");
    }
    pressEnterToContinue();
}

void viewMyMeals(int studentIdx) {
    printHeader("My Meal Bookings");

    const char *sid = students[studentIdx].studentID;
    int found = 0;
    int i;

    printf("  Student : %s  (ID: %s)\n",
           students[studentIdx].name, sid);
    printLine();

    for (i = 0; i < mealCount; i++) {
        if (strcmp(meals[i].studentID, sid) == 0) {
            printMealRecord(i);
            found++;
        }
    }

    if (!found)
        printf("  No meal bookings found.\n");

    pressEnterToContinue();
}

void studentMenu(int studentIdx) {
    int choice;

    while (1) {
        printHeader("Student Menu");
        printf("  Logged in as: %s\n\n", students[studentIdx].name);
        printf("  1. Book Meal\n");
        printf("  2. Update Meal\n");
        printf("  3. Cancel Meal\n");
        printf("  4. View My Meals\n");
        printf("  5. Logout\n");
        printLine();
        printf("  Enter your choice: ");
        if (!readInt(&choice)) choice = -1;

        switch (choice) {
            case 1: bookMeal(studentIdx);    break;
            case 2: updateMeal(studentIdx);  break;
            case 3: cancelMeal(studentIdx);  break;
            case 4: viewMyMeals(studentIdx); break;
            case 5:
                printf("\n  Logged out successfully. Goodbye, %s!\n",
                       students[studentIdx].name);
                pressEnterToContinue();
                return;
            default:
                printf("  [ERROR] Invalid choice. Try again.\n");
                pressEnterToContinue();
        }
    }
}

/* ---------- Admin / management ---------- */

int adminLogin(void) {
    printHeader("Management Login");

    char username[30], password[30];

    printf("Enter Admin Username: ");
    readLine(username, sizeof(username));

    printf("Enter Admin Password: ");
    readLine(password, sizeof(password));

    if (strcmp(username, ADMIN_USERNAME) == 0 &&
        strcmp(password, ADMIN_PASSWORD) == 0) {
        printf("\n[SUCCESS] Admin login successful.\n");
        return 1;
    }

    printf("[ERROR] Invalid admin credentials.\n");
    pressEnterToContinue();
    return 0;
}

void viewAllStudents(void) {
    printHeader("All Registered Students");

    if (studentCount == 0) {
        printf("  No students registered yet.\n");
        pressEnterToContinue();
        return;
    }

    int i;
    printf("  %-4s %-20s %-15s %-20s %-6s\n",
       "No.", "Name", "Student ID", "Discipline", "Room");
    printLine();

    for (i = 0; i < studentCount; i++) {
        printf("  %-4d %-20s %-15s %-20s %-6s\n",
       i + 1,
       students[i].name,
       students[i].studentID,
       students[i].discipline,
       students[i].roomNumber);
    }

    printf("\n  Total students: %d\n", studentCount);
    pressEnterToContinue();
}

void viewAllMealOrders(void) {
    printHeader("All Meal Orders");

    if (mealCount == 0) {
        printf("  No meal orders found.\n");
        pressEnterToContinue();
        return;
    }

    int i, j;

    for (i = 0; i < mealCount; i++) {
        int sIdx = findStudentByID(meals[i].studentID);
        const char *name = (sIdx != -1) ? students[sIdx].name : "Unknown";
        const char *room = (sIdx != -1) ? students[sIdx].roomNumber : "?";

        printf("\n  [%d] %s | %s | Room %s | Date: %s\n",
               i + 1, meals[i].studentID, name, room, meals[i].date);

        for (j = 0; j < 3; j++) {
            if (meals[i].meal[j]) {
                printf("      %-10s -> %s\n",
                       MEAL_NAMES[j],
                       STATUS_LABEL[meals[i].deliveryStatus[j]]);
            }
        }
    }

    pressEnterToContinue();
}

void viewTotalMealCount(void) {
    printHeader("Kitchen Meal Count Summary");

    char date[15];
    printf("Enter date (YYYY-MM-DD) or press Enter for ALL dates: ");
    readLine(date, sizeof(date));

    int bfTotal = 0, lnTotal = 0, dnTotal = 0;
    int i;

    for (i = 0; i < mealCount; i++) {
        if (strlen(date) == 0 || strcmp(meals[i].date, date) == 0) {
            bfTotal += meals[i].meal[BREAKFAST];
            lnTotal += meals[i].meal[LUNCH];
            dnTotal += meals[i].meal[DINNER];
        }
    }

    printf("\n");
    if (strlen(date) > 0)
        printf("  Date       : %s\n", date);
    else
        printf("  Date       : ALL DATES\n");

    printLine();
    printf("  Breakfast  : %d portions\n", bfTotal);
    printf("  Lunch      : %d portions\n", lnTotal);
    printf("  Dinner     : %d portions\n", dnTotal);
    printLine();
    printf("  TOTAL      : %d portions\n", bfTotal + lnTotal + dnTotal);
    pressEnterToContinue();
}

void viewDeliveryList(void) {
    printHeader("Food Delivery List");

    char date[15];
    printf("Enter date (YYYY-MM-DD) or Enter for ALL: ");
    readLine(date, sizeof(date));

    int found = 0;
    int i;

    printf("\n  %-15s %-20s %-6s %-10s %-30s\n",
           "Student ID", "Name", "Room", "Date", "Meals Ordered");
    printLine();

    for (i = 0; i < mealCount; i++) {
        if (strlen(date) > 0 && strcmp(meals[i].date, date) != 0)
            continue;

        if (!meals[i].meal[BREAKFAST] &&
            !meals[i].meal[LUNCH]     &&
            !meals[i].meal[DINNER])
            continue;

        int sIdx = findStudentByID(meals[i].studentID);
        const char *name = (sIdx != -1) ? students[sIdx].name : "Unknown";
        const char *room = (sIdx != -1) ? students[sIdx].roomNumber : "?";

        char mealList[60] = "";
        int j;
        for (j = 0; j < 3; j++) {
            if (meals[i].meal[j]) {
                if (strlen(mealList) > 0) strcat(mealList, ", ");
                strcat(mealList, MEAL_NAMES[j]);
            }
        }

        printf("  %-15s %-20s %-6s %-10s %-30s\n",
               meals[i].studentID, name, room,
               meals[i].date, mealList);
        found++;
    }

    if (!found) printf("  No delivery records found.\n");
    else printf("\n  Total deliveries: %d\n", found);

    pressEnterToContinue();
}

void updateDeliveryStatus(void) {
    printHeader("Update Delivery Status");

    char sid[20], date[15];
    printf("Enter Student ID : ");
    readLine(sid, sizeof(sid));

    printf("Enter Date (YYYY-MM-DD): ");
    readLine(date, sizeof(date));

    int mIdx = findMealRecord(sid, date);
    if (mIdx == -1) {
        printf("[ERROR] No booking found for Student %s on %s.\n", sid, date);
        pressEnterToContinue();
        return;
    }

    printf("\nCurrent booking:\n");
    printMealRecord(mIdx);

    printf("\nMark as Delivered? (1=Breakfast, 2=Lunch, 3=Dinner, 4=All, 0=Cancel): ");
    int choice;
    if (!readInt(&choice)) choice = -1;

    switch (choice) {
        case 1:
            if (meals[mIdx].meal[BREAKFAST])
                meals[mIdx].deliveryStatus[BREAKFAST] = 1;
            else printf("[INFO] Breakfast was not booked.\n");
            break;
        case 2:
            if (meals[mIdx].meal[LUNCH])
                meals[mIdx].deliveryStatus[LUNCH] = 1;
            else printf("[INFO] Lunch was not booked.\n");
            break;
        case 3:
            if (meals[mIdx].meal[DINNER])
                meals[mIdx].deliveryStatus[DINNER] = 1;
            else printf("[INFO] Dinner was not booked.\n");
            break;
        case 4:
            if (meals[mIdx].meal[BREAKFAST]) meals[mIdx].deliveryStatus[BREAKFAST] = 1;
            if (meals[mIdx].meal[LUNCH])     meals[mIdx].deliveryStatus[LUNCH]     = 1;
            if (meals[mIdx].meal[DINNER])    meals[mIdx].deliveryStatus[DINNER]    = 1;
            printf("[SUCCESS] All booked meals marked as delivered.\n");
            break;
        case 0:
            printf("Operation cancelled.\n");
            pressEnterToContinue();
            return;
        default:
            printf("[ERROR] Invalid choice.\n");
            pressEnterToContinue();
            return;
    }

    saveMeals();
    printf("[SUCCESS] Delivery status updated.\n");
    printMealRecord(mIdx);
    pressEnterToContinue();
}

void viewDeliveryReport(void) {
    printHeader("Delivery Status Report");

    int totalOrders = 0;
    int delivered    = 0;
    int pending       = 0;
    int i, j;

    printf("  %-15s %-10s %-10s %-12s %-10s\n",
           "Student ID", "Date", "Meal", "Status", "Room");
    printLine();

    for (i = 0; i < mealCount; i++) {
        int sIdx = findStudentByID(meals[i].studentID);
        const char *room = (sIdx != -1) ? students[sIdx].roomNumber : "?";

        for (j = 0; j < 3; j++) {
            if (meals[i].meal[j]) {
                totalOrders++;
                const char *status;
                if (meals[i].deliveryStatus[j]) {
                    status = STATUS_LABEL[1];
                    delivered++;
                } else {
                    status = STATUS_LABEL[0];
                    pending++;
                }

                printf("  %-15s %-10s %-10s %-12s %-10s\n",
                       meals[i].studentID,
                       meals[i].date,
                       MEAL_NAMES[j],
                       status,
                       room);
            }
        }
    }

    printLine();
    printf("  Total Orders : %d\n", totalOrders);
    printf("  Delivered    : %d\n", delivered);
    printf("  Pending      : %d\n", pending);

    pressEnterToContinue();
}

void managementMenu(void) {
    int choice;

    while (1) {
        printHeader("Management Panel");
        printf("  1. View All Students\n");
        printf("  2. View Meal Orders\n");
        printf("  3. Total Meal Count (Kitchen)\n");
        printf("  4. Delivery List\n");
        printf("  5. Update Delivery Status\n");
        printf("  6. Delivery Status Report\n");
        printf("  7. Logout\n");
        printLine();
        printf("  Enter your choice: ");
        if (!readInt(&choice)) choice = -1;

        switch (choice) {
            case 1: viewAllStudents();      break;
            case 2: viewAllMealOrders();    break;
            case 3: viewTotalMealCount();   break;
            case 4: viewDeliveryList();     break;
            case 5: updateDeliveryStatus(); break;
            case 6: viewDeliveryReport();   break;
            case 7:
                printf("\n  Admin logged out.\n");
                pressEnterToContinue();
                return;
            default:
                printf("  [ERROR] Invalid choice. Try again.\n");
                pressEnterToContinue();
        }
    }
}

int main(void) {
    loadStudents();
    loadMeals();

    int choice;

    while (1) {
        printHeader("Hall Meal Management System");
        printf("  1. Student Registration\n");
        printf("  2. Student Login\n");
        printf("  3. Management Login\n");
        printf("  4. Exit\n");
        printLine();
        printf("  Enter your choice: ");
        if (!readInt(&choice)) choice = -1;

        switch (choice) {
            case 1:
                registerStudent();
                break;

            case 2: {
                int idx = studentLogin();
                if (idx != -1)
                    studentMenu(idx);
                break;
            }

            case 3:
                if (adminLogin())
                    managementMenu();
                break;

            case 4:
                printf("\n  Thank you for using Hall Meal Management System.\n");
                printf("  Goodbye!\n\n");
                return 0;

            default:
                printf("  [ERROR] Invalid choice. Please enter 1-4.\n");
                pressEnterToContinue();
        }
    }

    return 0;
}