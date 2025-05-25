#include <bits/stdc++.h>
using namespace std;

struct Task {
    string name;
    int priority;
    int deadline;
    string description;
    vector<string> dependencies;
};

struct TaskOperation {
    enum Type { ADD, DELETE, MODIFY } type;
    Task beforeState;
    Task afterState;
    string taskName;
};

stack<TaskOperation> undoStack;
stack<TaskOperation> redoStack;

map<string, Task> taskMap;
map<string, int> inDegree;
map<string, vector<string>> adjList;
set<string> taskNames;
set<string> completedTasks;
vector<string> executionOrder;

struct Compare {
    bool operator()(const Task &a, const Task &b) {
        if (a.deadline == b.deadline) {
            if (a.priority == b.priority)
                return a.name > b.name; 
            return a.priority < b.priority; 
        }
        return a.deadline > b.deadline; 
    }
};

void displayTask(const Task& task) {
    cout << "Task: " << task.name
         << " | Priority: " << task.priority
         << " | Deadline: " << task.deadline
         << " | Description: " << task.description << endl;
}

void generateDOTFile() {
    ofstream dotFile("tasks.dot");
    dotFile << "digraph TaskGraph {\n";
    for (const auto &entry : taskMap) {
        dotFile << "    \"" << entry.first << "\" [label=\"" << entry.first 
                << "\\nPriority: " << entry.second.priority 
                << "\\nDeadline: " << entry.second.deadline << "\"]\n";
    }
    for (const auto &entry : adjList) {
        for (const string &dependent : entry.second) {
            dotFile << "    \"" << entry.first << "\" -> \"" << dependent << "\";\n";
        }
    }
    dotFile << "}\n";
    dotFile.close();
    system("\"C:\\Program Files\\Graphviz\\bin\\dot.exe\" -Tpng tasks.dot -o tasks.png");
}

bool createExecutionOrder() {
    priority_queue<Task, vector<Task>, Compare> tempHeap;
    map<string, int> tempInDegree = inDegree;
    executionOrder.clear();

    for (const auto &entry : taskMap) {
        if (tempInDegree[entry.first] == 0) {
            tempHeap.push(entry.second);
        }
    }

    while (!tempHeap.empty()) {
        Task current = tempHeap.top(); tempHeap.pop();
        executionOrder.push_back(current.name);
        for (const string &dependent : adjList[current.name]) {
            tempInDegree[dependent]--;
            if (tempInDegree[dependent] == 0) {
                tempHeap.push(taskMap[dependent]);
            }
        }
    }

    if (executionOrder.size() != taskMap.size()) {
        cout << "Error: Circular dependency detected!\n";
        executionOrder.clear();
        return false;
    }
    return true;
}

void deleteTaskInternal(const string& taskName) {
    taskMap.erase(taskName);
    taskNames.erase(taskName);
    inDegree.erase(taskName);
    adjList.erase(taskName);
    for (auto &entry : adjList) {
        entry.second.erase(remove(entry.second.begin(), entry.second.end(), taskName), entry.second.end());
    }
}

bool addTask(int t) {
    Task task;
    string depInput;
    vector<string> dependencies;

    cout << "Enter Task Name, Priority, Deadline: ";
    cin >> task.name >> task.priority >> task.deadline;
    cin.ignore();

    cout << "Enter Task Description: ";
    getline(cin, task.description);

    cout << "Enter Dependencies (comma-separated or 'none'): ";
    getline(cin, depInput);

    if (taskMap.find(task.name) != taskMap.end()) {
        cout << "Error: Task with name '" << task.name << "' already exists!\n";
        return false;
    }

    if (depInput != "none") {
        stringstream ss(depInput);
        string dep;
        while (getline(ss, dep, ',')) {
            dep.erase(remove_if(dep.begin(), dep.end(), ::isspace), dep.end());
            if (t != 1 && taskNames.find(dep) == taskNames.end()) {
                cout << "Error: Task '" << task.name << "' depends on non-existent task '" << dep << "'!\n";
                return false;
            }
            dependencies.push_back(dep);
        }
    }

    task.dependencies = dependencies;
    taskMap[task.name] = task;
    taskNames.insert(task.name);

    for (const string &dep : dependencies) {
        adjList[dep].push_back(task.name);
        inDegree[task.name]++;
    }

    if (inDegree.find(task.name) == inDegree.end()) {
        inDegree[task.name] = 0;
    }

    if (t != 1 && !createExecutionOrder()) {
        deleteTaskInternal(task.name);
        return false;
    }

    TaskOperation op = {TaskOperation::ADD, {}, task, task.name};
    undoStack.push(op);
    while (!redoStack.empty()) redoStack.pop();

    generateDOTFile();
    return true;
}

void deleteTask() {
    string taskName;
    cout << "Enter the task name to delete: ";
    cin >> taskName;

    if (taskMap.find(taskName) == taskMap.end()) {
        cout << "Error: Task '" << taskName << "' does not exist!\n";
        return;
    }

    TaskOperation op = {TaskOperation::DELETE, taskMap[taskName], {}, taskName};
    undoStack.push(op);
    while (!redoStack.empty()) redoStack.pop();

    deleteTaskInternal(taskName);
    createExecutionOrder();
    cout << "Task '" << taskName << "' deleted successfully.\n";
    generateDOTFile();
}

void reprioritizeTask() {
    string taskName;
    int newPriority;
    cout << "Enter the task name to change priority: ";
    cin >> taskName;

    if (taskMap.find(taskName) == taskMap.end()) {
        cout << "Error: Task '" << taskName << "' does not exist!\n";
        return;
    }

    cout << "Enter new priority: ";
    cin >> newPriority;

    TaskOperation op = {TaskOperation::MODIFY, taskMap[taskName], {}, taskName};
    taskMap[taskName].priority = newPriority;
    op.afterState = taskMap[taskName];
    undoStack.push(op);
    while (!redoStack.empty()) redoStack.pop();

    createExecutionOrder();
    cout << "Priority updated successfully.\n";
}

void undoLastOperation() {
    if (undoStack.empty()) {
        cout << "Nothing to undo!\n";
        return;
    }

    TaskOperation op = undoStack.top(); undoStack.pop();

    switch (op.type) {
        case TaskOperation::ADD:
            deleteTaskInternal(op.taskName);
            break;
        case TaskOperation::DELETE:
        case TaskOperation::MODIFY:
            taskMap[op.taskName] = op.beforeState;
            break;
    }

    redoStack.push(op);
    createExecutionOrder();
}

void displayExecutionOrder() {
    if (executionOrder.empty()) {
        cout << "No valid execution order available.\n";
        return;
    }
    cout << "\nOptimized Task Execution Order:\n";
    for (const string &taskName : executionOrder) {
        displayTask(taskMap[taskName]);
    }
}

void markTaskCompleted() {
    string taskName;
    cout << "Enter the task name to mark as completed: ";
    cin >> taskName;

    if (taskMap.find(taskName) == taskMap.end()) {
        cout << "Error: Task '" << taskName << "' does not exist!\n";
        return;
    }

    deleteTaskInternal(taskName);
    completedTasks.insert(taskName);
    createExecutionOrder();
    cout << "Task '" << taskName << "' marked as completed.\n";
    generateDOTFile(); 
}

void displayAllDependencies() {
    cout << "\nTask Dependencies:\n";
    for (const auto &entry : taskMap) {
        cout << "Task: " << entry.first << " | Dependencies: ";
        if (entry.second.dependencies.empty()) cout << "None";
        else for (const string &dep : entry.second.dependencies) cout << dep << " ";
        cout << endl;
    }
}

void searchTasks(const string& keyword) {
    cout << "\nSearch Results for '" << keyword << "':\n";
    for (const auto &task : taskMap) {
        if (task.first.find(keyword) != string::npos || 
            task.second.description.find(keyword) != string::npos) {
            displayTask(task.second);
        }
    }
}

vector<Task> filterTasksByPriority(int minPriority) {
    vector<Task> filteredTasks;
    for (const auto &task : taskMap) {
        if (task.second.priority >= minPriority) {
            filteredTasks.push_back(task.second);
        }
    }
    return filteredTasks;
}

int main() {
    int n;
    cout << "Enter number of tasks: ";
    cin >> n;
    cin.ignore();
    for (int i = 0; i < n; i++) {
        if (!addTask(1)) return 0;
    }
    if (!createExecutionOrder()) return 0;
    displayExecutionOrder();

    int choice;
    while (true) {
        cout << "\nTask Scheduler Menu:\n";
        cout << "1. Add Task\n2. Change Priority\n3. Display Execution Order\n4. Mark Task as Completed\n5. Delete Task\n6. Display All Dependencies\n7. Undo Last Operation\n8. Search Tasks\n9. Filter Tasks by Priority\n10. Exit\nEnter your choice: ";
        cin >> choice;
        switch (choice) {
            case 1: addTask(2); break;
            case 2: reprioritizeTask(); break;
            case 3: displayExecutionOrder(); break;
            case 4: markTaskCompleted(); break;
            case 5: deleteTask(); break;
            case 6: displayAllDependencies(); break;
            case 7: undoLastOperation(); break;
            case 8: {
                string keyword;
                cout << "Enter keyword to search: ";
                cin.ignore();
                getline(cin, keyword);
                searchTasks(keyword);
                break;
            }
            case 9: {
                int minPriority;
                cout << "Enter minimum priority: ";
                cin >> minPriority;
                vector<Task> filtered = filterTasksByPriority(minPriority);
                cout << "\nTasks with priority >= " << minPriority << ":\n";
                for (const Task& task : filtered) {
                    displayTask(task);
                }
                break;
            }
            case 10: cout << "Exiting program...\n"; return 0;
            default: cout << "Invalid choice! Please try again.\n";
        }
    }
}
