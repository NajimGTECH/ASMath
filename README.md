# 🚀 Asteroids (C++ / SFML)

A recreation of the retro **Asteroids** arcade game made with **C++** and **SFML 3.0.2**, using a fully custom **mathematics library**.

Developed by **Najim BAKKALI** and **Tristan GERMAIN**.

---

## 🎮 Gameplay

You control a spaceship drifting through space. Your goal: **destroy asteroids** and **survive as long as possible**.

**Controls:**
- **Z** : Move forward  
- **Q / D** : Rotate left / right  
- **SPACE** : Shoot bullets  

**Game Rules:**
- You start with **3 lives**.  
- Destroying asteroids gives you **points**.  
- Large asteroids **split into smaller fragments** when hit.  
- The game ends when you lose all your lives.  
- Press **R** to restart after Game Over.  

The ship and asteroids all wrap around the screen edges.

---

## 🧮 Custom Mathematics Library

This project uses a self-built mathematical library designed for **game development** and **2D/3D transformations**.  
It replaces external math dependencies and demonstrates the mathematical foundations of graphics and physics systems.

### Implemented Classes
- **Vector2\<T\>** – 2D vector class (positions, directions, velocities)  
- **Vector3\<T\>** – 3D vector class  
- **Mat3\<T\>** – 3×3 matrix (used for 2D rotation and transformations)  
- **Mat4\<T\>** – 4×4 matrix (useful for 3D projections)  
- **Quaternion** – represents 3D rotations without gimbal lock  

### Usage in the Game
These classes are used everywhere:
- **`Vector2`** handles positions, velocities, and direction logic for all entities.  
- **`Mat3`** builds rotation + translation matrices for rendering the player and asteroids.  
- **`Quaternion`, `Mat4`, and `Vector3`** provide 3D math support and extensibility for future projects.  

The goal is to demonstrate how **mathematical abstractions** can form the foundation of real-time gameplay mechanics.

---

## 🧱 Technologies Used
- **C++20**
- **SFML 3.0.2**
- **Visual Studio 2022**
- **Custom Math Library**

---

## 🧠 Documentation

The project is documented with **Doxygen**.  
You can generate the HTML documentation using:

```bash
doxygen Doxyfile
```
Open docs/html/index.html to view the complete documentation.
---

## 👥 Authors

Najim BAKKALI
Tristan GERMAIN

---

## 📜 License

This project is open-source for educational purposes.
You’re free to explore, fork, and learn from it.
