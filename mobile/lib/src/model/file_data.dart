class FileData {
  int? id;

  String fileName;
  String filePath;
  bool isSuccessful;
  int percentage;

  FileData(
      {this.id,
      required this.fileName,
      required this.filePath,
      required this.isSuccessful,
      required this.percentage});

  Map<String, dynamic> toMap() {
    return {
      'id': id,
      'fileName': fileName,
      'filePath': filePath,
      'isSuccessful': isSuccessful ? 1 : 0,
      'percentage': percentage
    };
  }

  factory FileData.fromMap(Map<String, dynamic> map) {
    return FileData(
        id: map['id'],
        fileName: map['fileName'],
        filePath: map['filePath'],
        isSuccessful: map['isSuccessful'] == 1,
        percentage: map['percentage']);
  }
}
