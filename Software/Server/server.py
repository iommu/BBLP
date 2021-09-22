#!/usr/bin/env python3
import pathlib
import json
from flask import Flask, Response, request
from flask_cors import CORS, cross_origin

app = Flask(__name__)

# Setup CORS
cors = CORS(app)
app.config['CORS_HEADERS'] = 'Content-Type'


@app.route("/")
@cross_origin()
def cors():
    return ""


# Define path to "pull" json file and check exists
question_path = pathlib.Path(__file__).parent.resolve() / "questions.json"
if not question_path.is_file():
    print("Error : questions.json file does not exist!")
    quit()


@app.route("/pull", methods=['GET'])
def pull():
    with open(question_path, "r") as question_file:
        return question_file.read()  # return read question json file as string


# Define path for student submissions to be saved to and create if doesn't exist
upload_path = pathlib.Path(__file__).parent.resolve() / "student_submissions"
pathlib.Path(upload_path).mkdir(parents=True, exist_ok=True)


@app.route('/push/<int:stu_id>', methods=['POST'])
def push(stu_id):
    # New path to where to save specific file
    submission_path = upload_path / f"{stu_id}.json"
    with open(submission_path, "a") as submission_file:
        # Get JSON as dict
        submission_json = request.json
        if submission_json:
            # Append a comma for JSON delimiting
            submission_file.write(",")
            # Append request dict to file as JSON string
            submission_file.write(json.dumps(submission_json))
        return Response(status=200)


if __name__ == "__main__":
    app.run(debug=True)
