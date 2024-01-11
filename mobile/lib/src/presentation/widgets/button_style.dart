import 'package:flutter/material.dart';

class ButtonStyles {
  Widget button(String text, VoidCallback onPressed, Color color) {
    return TextButton(
        style: TextButton.styleFrom(
          backgroundColor: color,
          foregroundColor: Colors.white,
          fixedSize: const Size(double.infinity, 50),
          shape:
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
        ),
        onPressed: () {
          onPressed();
        },
        child: Text(text, style: const TextStyle(fontSize: 16)));
  }

  Widget textButton(String text, VoidCallback onPressed, Color color) {
    return TextButton(
        style: TextButton.styleFrom(
          foregroundColor: color,
          fixedSize: const Size(double.infinity, 50),
          shape:
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
        ),
        onPressed: () {
          onPressed();
        },
        child: Text(text, style: const TextStyle(fontSize: 16)));
  }

  Widget colorButton(Color color, VoidCallback onPressed) {
    return TextButton(
        style: TextButton.styleFrom(
            backgroundColor: color,
            foregroundColor: Colors.white,
            fixedSize: const Size(double.infinity, 50),
            shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(10),
                side: const BorderSide(color: Colors.black, width: 1))),
        onPressed: () {
          onPressed();
        },
        child: const SizedBox());
  }

  Widget iconButton(Icon icon, VoidCallback onPressed) {
    return IconButton(
        onPressed: () {
          onPressed();
        },
        icon: icon,
        iconSize: 32);
  }
}
