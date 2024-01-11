import 'dart:io';
import 'package:Scanner3D/src/model/file_data.dart';
import 'package:path/path.dart';
import 'package:path_provider/path_provider.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart' as path;

class DatabaseHelper {
  static final DatabaseHelper instance = DatabaseHelper._privateConstructor();
  static Database? _database;
  DatabaseHelper._privateConstructor();

  Future<Database> get database async {
    if (_database != null) return _database!;
    _database = await _initDatabase();
    return _database!;
  }

  Future<Database> _initDatabase() async {
    Directory documentsDirectory = await getApplicationDocumentsDirectory();
    String path = join(documentsDirectory.path, 'scannings.db');
    return await openDatabase(path, version: 1, onCreate: _createTable);
  }

  Future<void> _createTable(Database db, int version) async {
    await db.execute('''
    CREATE TABLE file_data (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      fileName TEXT, 
      filePath TEXT, 
      isSuccessful INTEGER,
      percentage INTEGER
    )
  ''');
  }

  Future<int> insertFileData(FileData fileData) async {
    Database db = await instance.database;

    String baseFileName = path.basenameWithoutExtension(fileData.fileName);
    String fileExtension = path.extension(fileData.fileName);

    List<Map<String, dynamic>> existingFiles = await db.query(
      'file_data',
      where: 'fileName = ?',
      whereArgs: [fileData.fileName],
    );

    String uniqueFileName = fileData.fileName;
    int counter = 1;
    while (existingFiles.isNotEmpty) {
      uniqueFileName = "${baseFileName}_$counter$fileExtension";
      existingFiles = await db.query(
        'file_data',
        where: 'fileName = ?',
        whereArgs: [uniqueFileName],
      );
      counter++;
    }
    fileData.fileName = uniqueFileName;

    int id = await db.insert('file_data', fileData.toMap());
    return id;
  }

  Future<List<FileData>> getFileDataList() async {
    Database db = await instance.database;
    List<Map<String, dynamic>> maps = await db.query('file_data');
    return List.generate(maps.length, (index) {
      return FileData.fromMap(maps[index]);
    });
  }

  Future<void> deleteFileData(int id) async {
    Database db = await instance.database;
    await db.delete('file_data', where: 'id = ?', whereArgs: [id]);
  }
}
