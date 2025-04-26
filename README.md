# Django WebSocket Chat Application

This is a simple Django-based chat application that uses WebSockets for real-time communication between two clients (Client A and Client B). The application is powered by **Django Channels**, **Redis**, and **Daphne**.

## Features
- Real-time messaging between two clients.
- WebSocket-based communication.
- Messages are stored in the database for persistence.

---

## Prerequisites
Before running the application, ensure you have the following installed:
1. **Python 3.8+**
2. **Redis** (version 5.0 or higher)
3. **Daphne** (ASGI server)

---

## Setup Instructions

### 1. Clone the Repository
```bash
git clone <repository-url>
cd <repository-folder>
```

### 2. pip install -r requirements.txt

#### 3. Set Up Redis
On Windows:
Download Redis for Windows from Redis for Windows Releases. (https://github.com/tporadowski/redis/releases)
Extract the files and navigate to the Redis folder.
Start the Redis server
redis-server.exe


### 5. Configure the Django Project
Apply database migrations:
python manage.py makemigrations
python manage.py migrate

### 6. Run the Application with Daphne
Daphne is required to handle WebSocket connections. Install Daphne:
pip install daphne

Run the application using Daphne:
daphne -b 127.0.0.1 -p 8080 messaging.asgi:application

### 7. Run 
python manage.py runserver
*** IMPORTANT: CHANGE 8000 -> 8080 ***
` http://127.0.0.1:8080/a`